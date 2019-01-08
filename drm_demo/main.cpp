//
// Accelize DRM Demo
//
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <vector>
#include <thread>
#include <fcntl.h>
#include <ncurses.h>
#include <fstream>

#include <xclhal2.h>
#define SDX_KERNEL_BASE_ADDR	0x1800000
#define DRM_BASE_ADDRESS        (SDX_KERNEL_BASE_ADDR+0x100)
#define USER_IP_BASE_ADDRESS  	(SDX_KERNEL_BASE_ADDR+0x000)

#ifdef AWS
// FPGA Libs
#include <fpga_pci.h>
#include <fpga_mgmt.h>
//#define AGFI_STR	std::string("agfi-0c35f2c467fcbf7f7")
//#define DRM_BASE_ADDRESS        0x10000
//#define USER_IP_BASE_ADDRESS  	0x00000
#define FPGA_PROG_CMD std::string("sudo fpga-load-local-image -I agfi-0c35f2c467fcbf7f7 -S ")
#else
//#include <xclhal2.h>
//#define SDX_KERNEL_BASE_ADDR	0x1800000
//#define DRM_BASE_ADDRESS        (SDX_KERNEL_BASE_ADDR+0x100)
//#define USER_IP_BASE_ADDRESS  	(SDX_KERNEL_BASE_ADDR+0x000)
#define FPGA_PROG_CMD std::string("/opt/xilinx/xrt/bin/xbutil program -p bitstreams/u200/binary_container_1.xclbin -d ")
#endif

// Accelize DRMLib
#include <mutex>
#include "accelize/drm.h"
using namespace Accelize::DRMLib;
std::mutex gDrmMutex;

using namespace std;

// Terminal Colors
#define RESET   	    "\033[0m"
#define BOLDBLACK       "\033[1m\033[30m"    /* Bold Black */
#define BOLDRED         "\033[1m\033[31m"    /* Bold Red   */
#define BOLDYELLOW      "\033[1m\033[33m"    /* Bold Yellow   */
#define BOLDGREEN       "\033[1m\033[32m"    /* Bold Green */
#define BOLDWHITE       "\033[1m\033[37m"    /* Bold White */
#define BGBOLDRED       "\033[1m\033[41m"    /* Bold Red   */
#define BGBOLDGREEN     "\033[1m\033[42m"    /* Bold Green */
#define BGBOLDWHITE     "\033[1m\033[47m"    /* Bold White */

#define HLCOLOR         "\033[1m\033[30;47m"
#define OKCOLOR         "\033[1m\033[37;42m"
#define KOCOLOR         "\033[1m\033[37;41m"
#define LOADCOLOR       "\033[1m\033[37;43m"

#define MAX_NB_SLOTS            1
#define LOG_SIZE               10
#define SLOT_STATUS_DISABLED    0
#define SLOT_STATUS_ENABLED     1
#define LICENSE_MODE_NODELOCKED 0
#define LICENSE_MODE_FLOATING   1
#define LICENSE_MODE_METERED    2
#define IP_STATUS_UNKNOWN       0
#define IP_STATUS_STARTING      1
#define IP_STATUS_LOCKED        2
#define IP_STATUS_ACTIVATED     3
#define STATE_IDLE 		0
#define STATE_NO_LIC 	1
#define STATE_NO_SEAT	2
#define STATE_RUNNING	3
#define STATE_ERROR		4
#define FSM_IDLE 			0
#define FSM_INIT 			1
#define FSM_START_SESSION 	2
#define FSM_READ_IP_STATUS 	3
#define FSM_ERROR 			4

std::string gSlotStatusStr[]  = {std::string("DISABLED"), std::string("ENABLED")};
std::string gLicenseModeStr[] = {std::string("NODELOCKED"), /*std::string("NODELOCKED_PROV"),*/ std::string("FLOATING"), std::string("METERED")};
std::string gIpStatusStr[]    = {std::string(""), std::string("Starting ..."), std::string("LOCKED"), std::string("ACTIVATED")};
std::string gIpStatusColor[]  = {std::string(RESET), std::string(LOADCOLOR), std::string(KOCOLOR), std::string(OKCOLOR)};
std::string gDrmLibConfPath[] = {std::string("conf/nodelocked/"), /*std::string("conf/nodelocked_prov/"),*/ std::string("conf/floating/"), std::string("conf/metered/")};
std::string gSlotStateStr[] = {std::string(" "), std::string("NO_LIC"), std::string("NO_SEAT"), std::string("Running"), std::string("ERROR")};

// FPGA Design Defines
#define DRM_PAGE_REG_OFFSET     0x00000
#define DRM_STATUS_REG_OFFSET   0x00040
#define DRM_VERSION_REG_OFFSET  0x00068

typedef struct slotInfos {
	uint32_t slotID;
	uint32_t slotStatus;
	uint32_t licMode;
    uint32_t ipStatus;
    uint32_t slotState;
} slotInfos_t;

typedef struct highlightCell {
	uint32_t slotID;
	bool onLicenseMode;
} highlightCell_t;

typedef struct dashBoard {
    uint32_t nbSlots;
    slotInfos_t slot[MAX_NB_SLOTS];
    highlightCell_t hlCell;
} dashBoard_t;

dashBoard_t gDashboard;
bool bExit=false;   

char gSpinner1[] = {'|', '/', '-', '\\'};
char gSpinner2[] = {'.', 'o', 'O', 'o'};
char gSpinner3[] = {'V', '<', '^', '>'};
char gSpinner4[] = {'+', 'x', '+', 'x'};

std::string gRingBuffer[LOG_SIZE];
uint32_t gRBindex=0;
std::mutex gRBMutex;

void addToRingBuffer(uint32_t slotID, std::string newEntry, char* color=(char*)RESET)
{
	gRBMutex.lock();
	gRingBuffer[gRBindex] = std::string(color)  + std::string("Slot ") + std::to_string(slotID)  + std::string(": ") + newEntry + std::string(RESET);
	gRBindex = (gRBindex+1)%LOG_SIZE;	
	gRBMutex.unlock();
}

void printRB()
{
	gRBMutex.lock();
	// from index to end
	for(uint32_t i=gRBindex; i<LOG_SIZE; i++)
		printf("%-s\n\r", gRingBuffer[i].c_str());
	
	// from start to index
	for(uint32_t i=0; i<gRBindex; i++)
		printf("%-s\n\r", gRingBuffer[i].c_str());
	gRBMutex.unlock();
}

/*
 * DRMLib Read Callback Function
 */
int32_t my_read_drm(xclDeviceHandle handle, uint32_t  addr, uint32_t * value)
{	
    gDrmMutex.lock();
    int ret = (int)xclRead(handle, XCL_ADDR_KERNEL_CTRL, addr, value, 4);
    if(ret <= 0) {
        std::cout << __FUNCTION__ << ": Unable to read from the fpga ! ret = " << ret << std::endl;
        gDrmMutex.unlock();
        return 1;
    }
    gDrmMutex.unlock();
    return 0;
}

/*
 * DRMLib Write Callback Function
 */
int32_t my_write_drm(xclDeviceHandle handle, uint32_t  addr, uint32_t value)
{
    gDrmMutex.lock();
    int ret = (int)xclWrite(handle, XCL_ADDR_KERNEL_CTRL, addr, &value, 4);
    if(ret <= 0) {
        std::cout << __FUNCTION__ << ": Unable to write to the fpga ! ret=" << ret << std::endl;
        gDrmMutex.unlock();
        return 1;
    }
    gDrmMutex.unlock();
    return 0;
}

/*
 * Program_FPGA
 */
void progFPGA(uint32_t slotID)
{
	std::string cmd = FPGA_PROG_CMD + std::to_string(slotID) + std::string(" > /dev/null");
	system(cmd.c_str());
} 

std::string AccelizeLogo("\
     _    ____ ____ _____ _     ___ __________ \n\
    / \\  / ___/ ___| ____| |   |_ _|__  / ____|\n\
   / _ \\| |  | |   |  _| | |    | |  / /|  _|  \n\
  / ___ \\ |__| |___| |___| |___ | | / /_| |___ \n\
 /_/   \\_\\____\\____|_____|_____|___/____|_____|\n");
                                               

std::string  AccelizeLogoSmall("\
    _   ___ ___ ___ _    ___ _______   ___  ___ __  __   ___  ___ __  __  ___   \n\r\
   /_\\ / __/ __| __| |  |_ _|_  / __| |   \\| _ \\  \\/  | |   \\| __|  \\/  |/ _ \\ \n\r\
  / _ \\ (_| (__| _|| |__ | | / /| _|  | |) |   / |\\/| | | |) | _|| |\\/| | (_) | \n\r\
 /_/ \\_\\___\\___|___|____|___/___|___| |___/|_|_\\_|  |_| |___/|___|_|  |_|\\___/ \n\r");


void printDashLine(char colSep='+')
{
    for(uint32_t i=0; i<(gDashboard.nbSlots+1); i++) {
		printf("%c %12.12s ", colSep, "---------------------");
	}
	printf("%c\n\r", colSep);
}

void displayDashboard()
{
    uint32_t spinnerIndex = 0;
	while (!bExit) {
        clear();  
        refresh();
        
        // Print Header
        // - print accelize logo
        printf("%s\n\r", AccelizeLogoSmall.c_str());
        printDashLine();        
        printf("|%s  %c %c %c %c %c   %s", BOLDWHITE, gSpinner1[spinnerIndex%4], gSpinner2[spinnerIndex%4], gSpinner3[spinnerIndex%4], gSpinner4[spinnerIndex%4], gSpinner1[spinnerIndex%4], RESET);
        spinnerIndex++;
        // - Print Header Text
        int lineWidth = ((14+1)*gDashboard.nbSlots+1);
        int txtWidth  = strlen("FPGA Slots");
        int padding = (lineWidth/2) - (txtWidth/2) - 1;
        printf("| %s%*s%s%*s%s|\n\r", BOLDWHITE, padding, " ", "FPGA Slots", padding, " ", RESET);
        printDashLine();
        
        // Print Slots IDs
        printf("|%s %12.12s %s", BOLDWHITE, "Slot", RESET);        
        for(uint32_t i=0; i<gDashboard.nbSlots; i++) {
            std::string index = std::to_string(i);
            printf("| %12.12s ", index.c_str());
        }
        printf("|\n\r");        
        printDashLine();        
        
        // Print Slots Status        
        printf("|%s %12.12s %s", BOLDWHITE, "Slot Status", RESET);
        for(uint32_t i=0; i<gDashboard.nbSlots; i++) {
            const char* hlColor = (!gDashboard.hlCell.onLicenseMode && gDashboard.hlCell.slotID==i)?HLCOLOR:RESET;
            std::string status = gSlotStatusStr[gDashboard.slot[i].slotStatus];            
            printf("|%s %12.12s %s", hlColor, status.c_str(), RESET);
        }
        printf("|\n\r");        
        printDashLine();
        
        // Print Licensing Mode
        printf("|%s %12.12s %s", BOLDWHITE, "License Mode", RESET);
        for(uint32_t i=0; i<gDashboard.nbSlots; i++) {
            const char* hlColor = (gDashboard.hlCell.onLicenseMode && gDashboard.hlCell.slotID==i)?HLCOLOR:RESET;
            std::string mode = gLicenseModeStr[gDashboard.slot[i].licMode];
            printf("|%s %12.12s %s", hlColor, mode.c_str(), RESET);
        }
        printf("|\n\r");        
        printDashLine();
        
        // Print Licensing Status
        printf("|%s %12.12s %s", BOLDWHITE, "IP Status", RESET);
        for(uint32_t i=0; i<gDashboard.nbSlots; i++) {
            std::string ipstatus = gIpStatusStr[gDashboard.slot[i].ipStatus];
            std::string ipstatcolor = gIpStatusColor[gDashboard.slot[i].ipStatus];
            printf("|%s %12.12s %s", ipstatcolor.c_str(), ipstatus.c_str(), RESET);
        }
        printf("|\n\r");        
        printDashLine();
        
        // Print STATE
        printf("|%s %12.12s %s", BOLDWHITE, "STATE", RESET);
        for(uint32_t i=0; i<gDashboard.nbSlots; i++) {
			std::string state = gSlotStateStr[gDashboard.slot[i].slotState];
            printf("|%s %12.12s %s", RESET, state.c_str(), RESET);
        }
        printf("|\n\r");        
        printDashLine();
                
        // Print log
        printf("\n\r");
		printDashLine();
        printRB();
        printDashLine();
        
        sleep(1);
    }
    return;
}


void update(int key)
{    
    switch(key) {
        case KEY_UP:
        case KEY_DOWN:
            gDashboard.hlCell.onLicenseMode = !gDashboard.hlCell.onLicenseMode;
            break;
        case KEY_LEFT:
            if(gDashboard.hlCell.slotID == 0)
                gDashboard.hlCell.slotID = gDashboard.nbSlots-1;
            else
                gDashboard.hlCell.slotID -= 1;
            break;
        case KEY_RIGHT:
            if(gDashboard.hlCell.slotID == (gDashboard.nbSlots-1))
                gDashboard.hlCell.slotID = 0;
            else
                gDashboard.hlCell.slotID += 1;
            break;
        case ' ':
        case '\n':
            if(!gDashboard.hlCell.onLicenseMode) {
                gDashboard.slot[gDashboard.hlCell.slotID].slotStatus = !gDashboard.slot[gDashboard.hlCell.slotID].slotStatus;
                gDashboard.slot[gDashboard.hlCell.slotID].ipStatus = gDashboard.slot[gDashboard.hlCell.slotID].slotStatus?IP_STATUS_STARTING:IP_STATUS_UNKNOWN;
            }
            else {
                if(gDashboard.slot[gDashboard.hlCell.slotID].licMode == LICENSE_MODE_METERED)
                    gDashboard.slot[gDashboard.hlCell.slotID].licMode = 0;
                else
                    gDashboard.slot[gDashboard.hlCell.slotID].licMode += 1;
            }
            break;  
        
    }
}

/**
 * Keyb thread
 */
void KeybThread()
{
    while (!bExit) {
        timeout(1);
        nodelay(stdscr, TRUE);
        int key = getch();
        switch(key) {
            case ERR:
                break;
            case 'q':
            case 'Q':
                bExit = true;
                break;
            default:
                update(key);
                break;
        }
    }
    return;
}

/**
 * Init FPGA
 */
int32_t initFPGA(uint32_t slotID, xclDeviceHandle & handle)
{
    /* Program FPGA from selected slot */
	addToRingBuffer(slotID, std::string("[INFO] Programming FPGA ..."));	
	progFPGA(slotID);	

    if(xclProbe() < 1) {
        addToRingBuffer(slotID, std::string("[ERROR] xclProbe failed ..."));
        gDashboard.slot[slotID].slotState = STATE_ERROR;
		return -1;
	}
    addToRingBuffer(slotID, std::string("[INFO] xclProbe success ..."));
	
    std::string logFileName = std::string("xcl_run_") + std::to_string(slotID) + std::string(".log");
    xclVerbosityLevel level = XCL_ERROR;
    handle = xclOpen(slotID, logFileName.c_str(), level);
    if(handle == NULL) {
		addToRingBuffer(slotID, std::string("[ERROR] xclOpen failed ..."));
		gDashboard.slot[slotID].slotState = STATE_ERROR;
		return -1;
	}
    addToRingBuffer(slotID, std::string("[INFO] xclOpen success ..."));

	int32_t ret = xclLockDevice(handle);
    if(ret != 0) {
		addToRingBuffer(slotID, std::string("[ERROR] xclLockDevice failed ..."));
		gDashboard.slot[slotID].slotState = STATE_ERROR;
		return ret;
    }
    addToRingBuffer(slotID, std::string("[INFO] xclLockDevice success ..."));
    return 0;
}

/**
 * Uninit FPGA
 */
void uninitFPGA(uint32_t slotID, xclDeviceHandle handle)
{
    xclUnlockDevice(handle);
    xclClose(handle);
    addToRingBuffer(slotID, std::string("[INFO] Uninit FPGA ..."));
    gDashboard.slot[slotID].slotState = STATE_IDLE;
}


/**
 * Slot thread
 */
void SlotThread(uint32_t slotID)
{	
	uint32_t fsm = FSM_IDLE;
	MeteringSessionManager *pMSM;
	uint32_t savedLicMode = gDashboard.slot[slotID].licMode;
    xclDeviceHandle handle = NULL;
		
    while (!bExit) {

		// Handle change of lic mode or disabling during exec
		if(gDashboard.slot[slotID].licMode != savedLicMode || !gDashboard.slot[slotID].slotStatus) {
			savedLicMode = gDashboard.slot[slotID].licMode;
			if(fsm > FSM_INIT) {
				pMSM->stop_session();
				delete pMSM;
			}
			if(fsm > FSM_IDLE)
				uninitFPGA(slotID, handle);
			fsm = FSM_IDLE;	
		}
		
		switch(fsm) {
			case FSM_IDLE:
			default:
				gDashboard.slot[slotID].slotState=STATE_IDLE;
				if(gDashboard.slot[slotID].slotStatus) {
					// Program FPGA from selected slot
					addToRingBuffer(slotID, std::string("[INFO] Initialazing FPGA ..."));	
					//progFPGA(slotID);
                    if(initFPGA(slotID, handle))
                        fsm = FSM_IDLE;
                    else
					    fsm = FSM_INIT;
				}
				break;
				
			case FSM_INIT:
				addToRingBuffer(slotID, std::string("[INFO] Initializing Metering Session Manager ..."));	
								
				try {
					pMSM = new MeteringSessionManager(
						gDrmLibConfPath[gDashboard.slot[slotID].licMode] + std::string("conf.json"),
						gDrmLibConfPath[gDashboard.slot[slotID].licMode] + std::string("cred.json"),
						[&]( uint32_t  offset, uint32_t * value) { /*Read DRM register*/
							return  my_read_drm(handle, DRM_BASE_ADDRESS+offset, value);
						},
						[&]( uint32_t  offset, uint32_t value) { /*Write DRM register*/
							return my_write_drm(handle, DRM_BASE_ADDRESS+offset, value);
						},
						[&]( const  std::string & err_msg) {
						   std::cerr  << err_msg << std::endl;
						}
					);
				}
				catch (const std::exception& e) {						
					if(gDashboard.slot[slotID].licMode==LICENSE_MODE_NODELOCKED) {
						gDashboard.slot[slotID].slotState = STATE_NO_LIC;
						addToRingBuffer(slotID, std::string("[ERROR] No Local License File found"), (char*)BOLDYELLOW);	
					}
					else
						addToRingBuffer(slotID, std::string("[ERROR] Metering Session Manager Init Failed"), (char*)BOLDRED);	
					fsm = FSM_INIT;
                    break;
				}
				fsm = FSM_START_SESSION;
				break;
				
			case FSM_START_SESSION:	
				addToRingBuffer(slotID, std::string("[INFO] Starting Session ..."));				
				try {
					pMSM->auto_start_session();
					fsm = FSM_READ_IP_STATUS;
					break;			
				}
				catch (const std::exception& e) {
					//out << e.what() << std::endl;
					if(gDashboard.slot[slotID].licMode==LICENSE_MODE_FLOATING) {
						gDashboard.slot[slotID].slotState = STATE_NO_SEAT;
						addToRingBuffer(slotID, std::string("[ERROR] No Seat Available"), (char*)BOLDYELLOW);	
					}					
					if(gDashboard.slot[slotID].licMode==LICENSE_MODE_NODELOCKED) {
						gDashboard.slot[slotID].slotState = STATE_NO_LIC;
						addToRingBuffer(slotID, std::string("[ERROR] No Local License File found"), (char*)BOLDYELLOW);	
					}					
					if(gDashboard.slot[slotID].licMode==LICENSE_MODE_METERED) {
						gDashboard.slot[slotID].slotState = STATE_ERROR;
						addToRingBuffer(slotID, std::string("[ERROR] Start Metering Session Failed"), (char*)BOLDRED);	
					}
					delete pMSM;
					fsm = FSM_INIT;
				}
				break;
				
			case FSM_READ_IP_STATUS:
				try {
					gDashboard.slot[slotID].slotState=STATE_RUNNING;
					uint32_t reg =0;
					if(!gDashboard.slot[slotID].slotStatus) {
						pMSM->stop_session();
						delete pMSM;
                        uninitFPGA(slotID, handle);
						fsm = FSM_IDLE;
					}
					else {
						addToRingBuffer(slotID, std::string("[INFO] Reading IP Status ..."));
						my_read_drm(handle, USER_IP_BASE_ADDRESS, &reg);
						gDashboard.slot[slotID].ipStatus = (reg&0x01)?IP_STATUS_ACTIVATED:IP_STATUS_LOCKED;
					}
				}
				catch (const std::exception& e) {
					//out << e.what() << std::endl;
					if(gDashboard.slot[slotID].licMode==LICENSE_MODE_FLOATING) {
						gDashboard.slot[slotID].slotState = STATE_NO_SEAT;
						addToRingBuffer(slotID, std::string("[ERROR] No Seat Available"), (char*)BOLDYELLOW);	
					}										
					if(gDashboard.slot[slotID].licMode==LICENSE_MODE_METERED) {
						gDashboard.slot[slotID].slotState = STATE_ERROR;
						addToRingBuffer(slotID, std::string("[ERROR] Metering Session Failed"), (char*)BOLDRED);	
					}
				}
				break;
			
		}
        sleep(1);
    }

	// Clean Exit
    if(fsm > FSM_INIT) {
		pMSM->stop_session();
		delete pMSM;
	}
    if(fsm > FSM_IDLE)
	uninitFPGA(slotID, handle);
			
    return;
}


//std::string exec(const char* cmd) 
//{
//    char buffer[128];
//    std::string result = "";
//    FILE* pipe = popen(cmd, "r");
//
//	while (fgets(buffer, sizeof buffer, pipe) != NULL)
//		result += buffer;
//    pclose(pipe);
//    return result;
//}


 
/*
 * Entry point
 */
int main(int argc, char **argv) 
{       	
	memset(&gDashboard, 0, sizeof(gDashboard));
    /* Initialize FPGA Communication */
    //fpga_pci_init();
    
    /* Detected number of FPGA slots available */
    /*unsigned nbDevices = xclProbe();
    printf("nbDevices = %u ======>>>>> TODO\n", nbDevices);
    if(nbDevices <= 0) {
		printf("==> Error: xclProbe failed, no device detected \n");
		return -1;
	}*/
    
    // NCurses init
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    
    // argument parsing : TODO
    gDashboard.nbSlots = MAX_NB_SLOTS;
    
    thread slot_threads[MAX_NB_SLOTS];
    for(uint32_t i=0; i<gDashboard.nbSlots; i++)
        slot_threads[i] = thread(SlotThread, i);
    
    // Start Threads
    thread keyb_thread(KeybThread);
    thread dashboard_thread(displayDashboard);
    
    keyb_thread.join();
    dashboard_thread.join();
    
    for(uint32_t i=0; i<gDashboard.nbSlots; i++)
        slot_threads[i].join();
    
    // NCurses exit
    endwin();
	return 0;
}

