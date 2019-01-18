/*
Copyright (C) 2018, Accelize
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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

// Accelize DRMLib
#include <mutex>
#include "accelize/drm.h"
using namespace Accelize::DRMLib;
std::mutex gDrmMutex;
bool gDebugMode = false;
using namespace std;

// Terminal Colors
#define RESET           "\033[0m"
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

#define FCELLSIZE               15
#define CELLSIZE                12
#define MAX_NB_SLOTS            8
#define LOG_SIZE               10
#define SLOT_STATUS_DISABLED    0
#define SLOT_STATUS_ENABLED     1
#define IP_STATUS_UNKNOWN       0
#define IP_STATUS_STARTING      1
#define IP_STATUS_LOCKED        2
#define IP_STATUS_ACTIVATED     3
#define STATE_IDLE              0
#define STATE_NO_LIC            1
#define STATE_NO_SEAT           2
#define STATE_RUNNING           3
#define STATE_ERROR             4
#define FSM_IDLE                0
#define FSM_INIT                1
#define FSM_START_SESSION       2
#define FSM_READ_IP_STATUS      3
#define FSM_ERROR               4

enum userNameIdx{ nancyIdx=0, fannyIdx, maryIdx, userIdx, nbIdx};
uint32_t gUserNameIndex=maryIdx;
bool gFullScreenMode=false;
std::string gAllowedUsers[]     = {std::string("nancy@acme.com"), std::string("fanny@acme.com"), std::string("mary@acme.com"), std::string("user@company.com")};
std::string gLicenseModeStr[]   = {std::string("NODELOCKED"), std::string("FLOATING (3 seats)"), std::string("METERED"), std::string("CUSTOM")};
std::string gAppStatusStr[]     = {std::string(" - "), std::string("STARTED")};
std::string gIpStatusStr[]      = {std::string(""), std::string("REQUESTING"), std::string("LOCKED"), std::string("ACTIVATED")};
std::string gIpStatusColor[]    = {std::string(RESET), std::string(LOADCOLOR), std::string(KOCOLOR), std::string(OKCOLOR)};
std::string gSlotStateStr[]     = {std::string(" "), std::string("NO_LIC"), std::string("NO_SEAT"), std::string("RUNNING"), std::string("ERROR")};

enum {
    LICENSE_MODE_NODELOCKED=0,
    LICENSE_MODE_FLOATING,
    LICENSE_MODE_METERED,
    LICENSE_UNKNOWN
};

//#define DRMDEMO_PATH std::string("/opt/accelize/drm_demo/")
#define DRMDEMO_PATH std::string("./")
#ifdef NLPROV
std::string gDrmLibConfPath[] = {std::string("conf/nancy_prov/"), std::string("conf/fanny/"), std::string("conf/mary/"), std::string("conf/user/")};
#else
std::string gDrmLibConfPath[] = {std::string("conf/nancy/"), std::string("conf/fanny/"), std::string("conf/mary/"), std::string("conf/user/")};
#endif

// FPGA Design Defines
#define DRM_PAGE_REG_OFFSET     0x00000
#define DRM_STATUS_REG_OFFSET   0x00040
#define DRM_VERSION_REG_OFFSET  0x00068

typedef struct slotInfos {
    uint32_t slotID;
    uint32_t slotStatus;
    uint32_t ipStatus;
    uint32_t slotState;
} slotInfos_t;

typedef struct highlightCell {
    uint32_t slotID;
} highlightCell_t;

typedef struct dashBoard {
    uint32_t nbSlots;
    slotInfos_t slot[MAX_NB_SLOTS];
    highlightCell_t hlCell;
} dashBoard_t;

char gSpinner1[] = {'.', 'o', 'O', 'o', '.', '.', '.', '.'};
char gSpinner2[] = {'.', '.', 'o', 'O', 'o', '.', '.', '.'};
char gSpinner3[] = {'.', '.', '.', 'o', 'O', 'o', '.', '.'};
char gSpinner4[] = {'.', '.', '.', '.', 'o', 'O', 'o', '.'};
char gSpinner5[] = {'.', '.', '.', '.', '.', 'o', 'O', 'o'};
uint32_t maxSpinIdx = sizeof(gSpinner1);

dashBoard_t gDashboard;
bool bExit=false;   
std::string gRingBuffer[LOG_SIZE];
uint32_t gRBindex=0;
std::mutex gRBMutex;

std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}

int32_t sysCommand(std::string cmd, std::string & output) {
    char buffer[128];
    output = "";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe){
        printf("sysCommand: [%s] error\n", cmd.c_str());
        return 1;
    } 
    
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            output += buffer;
        }
    } catch (...) {
        pclose(pipe);
        printf("sysCommand: [%s] error\n", cmd.c_str());
        return 1;
    }
    pclose(pipe);
    rtrim(output);
    return 0;
}


void setUserName(std::string userName)
{
    for(uint32_t i=0; i<nbIdx; i++) {
        if(gAllowedUsers[i] == userName) {
            gUserNameIndex=i;
            return;
        }
    }
    gAllowedUsers[userIdx] = userName;
    gUserNameIndex=userIdx;
}

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


void saveInLogFile(const char* label, uint32_t value)
{
    std::string txt = std::string("echo ") + std::string(label) + std::string("=[") + to_string(value) + std::string("] >> data.log");
    system(txt.c_str());
}

void saveInLogFile(const char* label, std::string value)
{
    std::string txt = std::string("echo ") + std::string(label) + std::string("=[") + value + std::string("] >> data.log");
    system(txt.c_str());
}

std::string centered(int width, const string& str) {
    int len = str.length();
    if(width < len) { return str.c_str(); }

    int diff = width - len;
    int pad1 = diff/2;
    int pad2 = diff - pad1;
    return (string(pad1, ' ') + str + string(pad2, ' '));
}

void printEmptyLine(void)
{
    if(gFullScreenMode) {
        printf("%c %*.*s ", '|', FCELLSIZE, FCELLSIZE, "                                   ");
        for(uint32_t i=0; i<gDashboard.nbSlots; i++) {
            printf("%c %*.*s ", ' ', CELLSIZE, CELLSIZE, "                                     ");
        }
        printf("%c\n\r", '|');  
    }
}

void printTabEmptyLine(char colSep='|', bool statColorize=false, bool cellHL=false)
{
    if(gFullScreenMode) {
        printf("%c %*.*s ", '|', FCELLSIZE, FCELLSIZE, "                                   ");
        for(uint32_t i=0; i<gDashboard.nbSlots; i++) {
            std::string statcolor = statColorize?(gIpStatusColor[gDashboard.slot[i].ipStatus]):RESET;
            std::string cellcolor = cellHL?((gDashboard.hlCell.slotID==i)?HLCOLOR:statcolor):statcolor;
            printf("%c%s %*.*s %s", colSep, cellcolor.c_str(), CELLSIZE, CELLSIZE, "                                     ", RESET);
        }
        printf("%c\n\r", '|');  
    }
}

void printDashLine(char colSep='+')
{   
    printf("%c %*.*s ", colSep, FCELLSIZE, FCELLSIZE, "---------------------------------");
    for(uint32_t i=0; i<gDashboard.nbSlots; i++) {
        printf("%c %*.*s ", colSep, CELLSIZE, CELLSIZE, "---------------------------------");
    }
    printf("%c\n\r", colSep);   
}


void displayDashboard()
{
    uint32_t spinnerIndex = 0;
    while (!bExit) {
        clear();  
        refresh();
        clear();  
        refresh();
        
        // Print Header
        // - print accelize logo
        printf("%s\n\r", AccelizeLogoSmall.c_str());     
        printf("%s  %c %c %c %c %c %s\n\r\n\r", BOLDWHITE, gSpinner1[spinnerIndex%maxSpinIdx], gSpinner2[spinnerIndex%maxSpinIdx], gSpinner3[spinnerIndex%maxSpinIdx], gSpinner4[spinnerIndex%maxSpinIdx], gSpinner5[spinnerIndex%maxSpinIdx], RESET);
        spinnerIndex++;
        // - print User Name
        printDashLine();
        printEmptyLine();
        std::string label = centered(FCELLSIZE + (CELLSIZE+3)*(gDashboard.nbSlots), gAllowedUsers[gUserNameIndex] + std::string(" - ") + gLicenseModeStr[gUserNameIndex]);
        printf("| %s%s%s |\n\r", BOLDWHITE, label.c_str(), RESET);
        printEmptyLine();
        printDashLine();        
        
        // Print Slots IDs      
        printTabEmptyLine('|');
        printf("|%s %s %s", BOLDWHITE, centered(FCELLSIZE, "FPGA Board").c_str(), RESET);        
        for(uint32_t i=0; i<gDashboard.nbSlots; i++) {
            std::string index = std::to_string(i+1);
            printf("| %s ", centered(CELLSIZE, index).c_str());
        }
        printf("|\n\r");
        printTabEmptyLine('|');        
        printDashLine();        
        
        // Print On/Off Status  
        printTabEmptyLine('|', false, true);      
        printf("|%s %s %s", BOLDWHITE, centered(FCELLSIZE, "App On/Off").c_str(), RESET);        
        for(uint32_t i=0; i<gDashboard.nbSlots; i++) {
            const char* hlColor = (gDashboard.hlCell.slotID==i)?HLCOLOR:RESET;
            std::string status = gAppStatusStr[gDashboard.slot[i].slotStatus];            
            printf("|%s %s %s", hlColor, centered(CELLSIZE, status).c_str(), RESET);
        }
        printf("|\n\r");    
        printTabEmptyLine('|', false, true);    
        printDashLine();
        
        // Print App Status
        printTabEmptyLine('|', true, false);
        printf("|%s %s %s", BOLDWHITE, centered(FCELLSIZE, "DRM Status").c_str(), RESET);
        for(uint32_t i=0; i<gDashboard.nbSlots; i++) {
            std::string ipstatus = gIpStatusStr[gDashboard.slot[i].ipStatus];
            std::string ipstatcolor = gIpStatusColor[gDashboard.slot[i].ipStatus];
            printf("|%s %s %s", ipstatcolor.c_str(), centered(CELLSIZE, ipstatus).c_str(), RESET);
        }
        printf("|\n\r");   
        printTabEmptyLine('|', true, false);     
        printDashLine();
        
        // Print STATE
        printTabEmptyLine('|');
        printf("|%s %s %s", BOLDWHITE, centered(FCELLSIZE, "App Status").c_str(), RESET);
        for(uint32_t i=0; i<gDashboard.nbSlots; i++) {
            std::string state = gSlotStateStr[gDashboard.slot[i].slotState];
            printf("|%s %s %s", RESET, centered(CELLSIZE, state).c_str(), RESET);
        }
        printf("|\n\r"); 
        printTabEmptyLine('|');       
        printDashLine();
                      
        // Print log
        if(!gFullScreenMode) {
            printf("\n\r");
            printDashLine();
            printRB();
            printDashLine();
        }      
        sleep(1);
    }
    return;
}

/**
 * 
 */
void update(void)
{
    gDashboard.slot[gDashboard.hlCell.slotID].slotStatus = !gDashboard.slot[gDashboard.hlCell.slotID].slotStatus;
    gDashboard.slot[gDashboard.hlCell.slotID].ipStatus = gDashboard.slot[gDashboard.hlCell.slotID].slotStatus?IP_STATUS_STARTING:IP_STATUS_UNKNOWN;     
}

/**
 * 
 */
void updateCellIdFromMouseCoord(uint32_t x, uint32_t y) 
{   
    uint32_t prevSlotId = gDashboard.hlCell.slotID;
    uint32_t exitCellYmin=0, exitCellYmax=6;
    uint32_t unameCellYmin=7, unameCellYmax=gFullScreenMode?11:9;
    
    // Exit hidden feature
    if(y>=exitCellYmin && y<=exitCellYmax) bExit = true;
        
    // Switch user hidden feature (only when all slots are OFF)
    if(y>=unameCellYmin && y<=unameCellYmax) {
        for(uint32_t i=0; i<gDashboard.nbSlots; i++)
            if(gDashboard.slot[i].slotStatus)
                return; 
        if(gUserNameIndex==userIdx) // switch disabled when user account used
            return;
            
        gUserNameIndex+=1;
        if(gUserNameIndex==userIdx) // Skip user account in demo mode
            gUserNameIndex=nancyIdx;    
        return;
    }
    
    // Update slot ID and start operation
    if(x < (FCELLSIZE+4))
        gDashboard.hlCell.slotID = 0;
    else if(x> ((FCELLSIZE+4) + gDashboard.nbSlots*(CELLSIZE+3)))
        gDashboard.hlCell.slotID = gDashboard.nbSlots-1;
    else
        gDashboard.hlCell.slotID = (x-(FCELLSIZE+4)) / (CELLSIZE+3);
        
    if( gDashboard.hlCell.slotID == prevSlotId)
        update();
}

/**
 * Input Events thread
 */
void InputEventsThread()
{   
    /* Get all the mouse events */
    MEVENT event;
    mousemask(BUTTON1_CLICKED|REPORT_MOUSE_POSITION, NULL);
    
    while (!bExit) {
        timeout(1);
        nodelay(stdscr, TRUE);
        int key = getch();
        switch(key) {
            case KEY_MOUSE:
                if(getmouse(&event)==OK)
                    updateCellIdFromMouseCoord(event.x, event.y);           
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
                update();
                break;              
            case ERR:
                break;
            case 'q':
            case 'Q':
                bExit = true;
                break;
            default:
                break;
        }
    }
    return;
}
