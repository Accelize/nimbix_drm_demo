// Amazon FPGA Hardware Development Kit
//
// Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Amazon Software License (the "License"). You may not use
// this file except in compliance with the License. A copy of the License is
// located at
//
//    http://aws.amazon.com/asl/
//
// or in the "license" file accompanying this file. This file is distributed on
// an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or
// implied. See the License for the specific language governing permissions and
// limitations under the License.

#ifdef AWS
#include "awshdk/hal.h"
#else
#include "xclhal2/hal.h"
#endif

/**
 * Slot thread
 */
void SlotThread(uint32_t slotID)
{	
	uint32_t fsm = FSM_IDLE;
	MeteringSessionManager *pMSM;
	uint32_t savedSlotStatus = gDashboard.slot[slotID].slotStatus;
		
    while (!bExit) {
	
		// Handle change of lic mode or disabling during exec		
		if( !gDashboard.slot[slotID].slotStatus && savedSlotStatus ) {
			if(fsm > FSM_INIT) {
				pMSM->stop_session();
				delete pMSM;
			}
			if(fsm > FSM_IDLE)
				uninitFPGA(slotID);
			addToRingBuffer(slotID, std::string("[INFO] Ready ..."));
			fsm = FSM_IDLE;	
		}
		
		savedSlotStatus = gDashboard.slot[slotID].slotStatus;
		
		switch(fsm) {
			case FSM_IDLE:
			default:
				gDashboard.slot[slotID].slotState=STATE_IDLE;
				if(gDashboard.slot[slotID].slotStatus) {
					addToRingBuffer(slotID, std::string("[INFO] Initializing FPGA ..."));	
                    if(initFPGA(slotID)) {
						addToRingBuffer(slotID, std::string("[ERROR] FPGA Initialization"), (char*)BOLDRED);
						gDashboard.slot[slotID].slotStatus = !gDashboard.slot[slotID].slotStatus;	
						gDashboard.slot[gDashboard.hlCell.slotID].ipStatus = 0;
                        fsm = FSM_IDLE;
                    }
                    else
					    fsm = FSM_INIT;
				}
				break;
				
			case FSM_INIT:
				addToRingBuffer(slotID, std::string("[INFO] Initializing Metering Session Manager ..."));	
								
				try {
					pMSM = new MeteringSessionManager(
						DRMDEMO_PATH + gDrmLibConfPath[gUserNameIndex] + std::string("conf.json"),
						DRMDEMO_PATH + gDrmLibConfPath[gUserNameIndex] + std::string("cred.json"),
						[&]( uint32_t  offset, uint32_t * value) { /*Read DRM register*/
							return  my_read_drm(slotID, DRM_BASE_ADDRESS+offset, value);
						},
						[&]( uint32_t  offset, uint32_t value) { /*Write DRM register*/
							return my_write_drm(slotID, DRM_BASE_ADDRESS+offset, value);
						},
						[&]( const  std::string & err_msg) {
						   std::cerr  << err_msg << std::endl;
						}
					);
				}
				catch (const std::exception& e) {						
					if(gUserNameIndex==LICENSE_MODE_NODELOCKED) {
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
					if(gUserNameIndex==LICENSE_MODE_FLOATING) {
						gDashboard.slot[slotID].slotState = STATE_NO_SEAT;
						addToRingBuffer(slotID, std::string("[ERROR] No Seat Available"), (char*)BOLDYELLOW);	
					}					
					if(gUserNameIndex==LICENSE_MODE_NODELOCKED) {
						gDashboard.slot[slotID].slotState = STATE_NO_LIC;
						addToRingBuffer(slotID, std::string("[ERROR] No Local License File found"), (char*)BOLDYELLOW);	
					}					
					if(gUserNameIndex==LICENSE_MODE_METERED) {
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
                        uninitFPGA(slotID);
                        addToRingBuffer(slotID, std::string("[INFO] Ready ..."));
						fsm = FSM_IDLE;
					}
					else {
						addToRingBuffer(slotID, std::string("[INFO] Reading IP Status ..."));
						my_read_drm(slotID, USER_IP_BASE_ADDRESS, &reg);
						gDashboard.slot[slotID].ipStatus = (reg&0x01)?IP_STATUS_ACTIVATED:IP_STATUS_LOCKED;
					}
				}
				catch (const std::exception& e) {
					//out << e.what() << std::endl;
					if(gUserNameIndex==LICENSE_MODE_FLOATING) {
						gDashboard.slot[slotID].slotState = STATE_NO_SEAT;
						addToRingBuffer(slotID, std::string("[ERROR] No Seat Available"), (char*)BOLDYELLOW);	
					}										
					if(gUserNameIndex==LICENSE_MODE_METERED) {
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
		uninitFPGA(slotID);
			
    return;
}


/*
 * show_usage
 */
 void show_usage(char* argv[])
 {
    std::cerr << "Usage: " << argv[0] << std::endl;
    std::cerr << "" << std::endl;
    std::cerr << "\t-n,--nb_slots      number of slots (default=8)" << std::endl;
    std::cerr << "\t-u,--user      	   username [mary, fanny, nancy] (default=mary)" << std::endl;
    std::cerr << "\t-f,--fullscreen    full-screen mode (default=off)" << std::endl;
    std::cerr << "\t-d,--debug    	   enable debug mode (default=off)" << std::endl;
    std::cerr << ""  << std::endl; 
 }


/**
 *  Parse Command Line Arguments
 */
int parse_cmdline_arguments(int argc, char*argv[])
{
	std::string userName("mary@bigcorp.com");
    const char* const short_opts = "n:u:vdfh?";
    const option long_opts[] = {
            {"nb_slots", required_argument, nullptr, 'n'},
            {"user", required_argument, nullptr, 'u'},
            {"fullscreen", no_argument, nullptr, 'f'},
            {"verbose", no_argument, nullptr, 'v'},
            {"debug", no_argument, nullptr, 'd'},
            {"help", no_argument, nullptr, 'h'},
            {nullptr, no_argument, nullptr, 0}
    };    
    
    while(true) {
        const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);
        if (-1 == opt)
            break;
            
        switch(opt) {
            case 'n':
                gDashboard.nbSlots = std::stoi(optarg);
                break;
            case 'u':
                userName = std::string(optarg);
                break;
            case 'f':
                gFullScreenMode=true;
                break;            
            case 'v':
                setenv("ACCELIZE_DRMLIB_VERBOSE", "5", 1);
                break;
            case 'd':
                gDebugMode=true;
                break;
            case 'h':
            case '?':
            case ' ':
            default:
                show_usage(argv);
                return 1;
                break;
        }
    } 
    
    if(userName.find('@')==std::string::npos) 
		userName += std::string("@bigcorp.com");
	
	if(!inWhiteList(userName)) {
		std::cout << "ERROR: User [" << userName << "] is not in the whitelist " << std::endl;
		return 1;
	}
    
    if(gDashboard.nbSlots > MAX_NB_SLOTS) {
		std::cout << "ERROR: Maximum number of slots is " << MAX_NB_SLOTS << std::endl;
		show_usage(argv);
		return 1;
	}
    
    return 0;
}

/**
 * 
 */
int32_t debugMode(uint32_t slotID)
{
    std::string user = gAllowedUsers[gUserNameIndex];
    printf("Starting debugMode with user %s\n", user.c_str());
	MeteringSessionManager *pMSM;
	printf("[%s] Init FPGA ..\n", user.c_str());
	initFPGA(slotID);
	
	printf("[%s] Start MSM ..\n", user.c_str());
	pMSM = new MeteringSessionManager(
		DRMDEMO_PATH + gDrmLibConfPath[gUserNameIndex] + std::string("conf.json"),
		DRMDEMO_PATH + gDrmLibConfPath[gUserNameIndex] + std::string("cred.json"),
		[&]( uint32_t  offset, uint32_t * value) { /*Read DRM register*/
			return  my_read_drm(slotID, DRM_BASE_ADDRESS+offset, value);
		},
		[&]( uint32_t  offset, uint32_t value) { /*Write DRM register*/
			return my_write_drm(slotID, DRM_BASE_ADDRESS+offset, value);
		},
		[&]( const  std::string & err_msg) {
		   std::cerr  << err_msg << std::endl;
		}
	);
	sleep(2);
	
	printf("[%s] Start Session ..\n", user.c_str());
	pMSM->auto_start_session();
    printf("[%s] Press enter to stop session\n", user.c_str());
    getchar();
	
	printf("[%s] Stop Session ..\n", user.c_str());
	pMSM->stop_session();
	
	printf("[%s] Uninit FPGA ..\n", user.c_str());
	uninitFPGA(slotID);	
	
	return 0;
}


/**
 * 
 */
int main(int argc, char **argv) 
{       	
	memset(&gDashboard, 0, sizeof(gDashboard));
	gDashboard.nbSlots = MAX_NB_SLOTS;
	
	/* Retrieve input arguments */
    if(parse_cmdline_arguments(argc, argv))
        return 1;
        
    /* HAL Sanity Check */
    if(halSanityCheck())
		return 1;
        
    if(gDebugMode) {
		setenv("ACCELIZE_DRMLIB_VERBOSE", "4", 1);
        return debugMode(0);
    }
    
    // NCurses init
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    
    thread slot_threads[MAX_NB_SLOTS];
    for(uint32_t i=0; i<gDashboard.nbSlots; i++)
        slot_threads[i] = thread(SlotThread, i);
    
    // Start Threads
    thread inputevents_thread(InputEventsThread);
    thread dashboard_thread(displayDashboard);
    
    inputevents_thread.join();
    dashboard_thread.join();
    
    for(uint32_t i=0; i<gDashboard.nbSlots; i++)
        slot_threads[i].join();
    
    // NCurses exit
    endwin();
	return 0;
}

