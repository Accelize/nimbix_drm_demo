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
#include <mutex>
#include <time.h>
#include <sstream>

bool gDebugMode = false;
using namespace std;

typedef enum {
    NOCOLOR     = 0,
    HIGHLIT_COLOR,
    BOLDWHITE_COLOR,
    ERROR_COLOR,
    SUCESS_COLOR,
    WARNING_COLOR,
    LOCKED_COLOR,
    UNLOCKED_COLOR
} color_t;

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
#define FSM_RUNNING             3
#define FSM_ERROR               4

enum userNameIdx{ nancyIdx=0, fannyIdx, maryIdx, userIdx, nbIdx};
uint32_t gUserNameIndex=maryIdx;
bool gFullScreenMode=false;
bool gGuiMode=false;
bool gQuietMode=false;
std::string gAllowedUsers[]    = {std::string("nancy@acme.com"), std::string("fanny@acme.com"), std::string("mary@acme.com"), std::string("user@company.com")};
std::string gLicenseModeStr[]  = {std::string("NODELOCKED"), std::string("FLOATING (3 seats)"), std::string("METERED"), std::string("CUSTOM")};
std::string gAppStatusStr[]    = {std::string(" - "), std::string("STARTED")};
std::string gDrmStatusStr[]    = {std::string(""), std::string("REQUESTING"), std::string("LOCKED"), std::string("ACTIVATED")};
//std::string gAppOutputStr[]    = {std::string(""), std::string(""), std::string(""), std::string("HelloWorld!")};
color_t gDrmStatusColor[]     = {NOCOLOR, WARNING_COLOR, LOCKED_COLOR, UNLOCKED_COLOR};
std::string gAppStateStr[]     = {std::string(" "), std::string("NO_LIC"), std::string("NO_SEAT"), std::string("RUNNING"), std::string("ERROR")};

enum {
    LICENSE_MODE_NODELOCKED=0,
    LICENSE_MODE_FLOATING,
    LICENSE_MODE_METERED,
    LICENSE_UNKNOWN
};

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
    bool slotOn;
    uint32_t drmStatus;
    uint32_t appState;
    std::string appOutput;
    std::string sessionID;
    std::string usageUnits;
} slotInfos_t;

typedef struct highlightCell {
    uint32_t slotID;
} highlightCell_t;

typedef struct dashBoard {
    uint32_t nbSlots;
    slotInfos_t slot[MAX_NB_SLOTS];
    highlightCell_t hlCell;
} dashBoard_t;

dashBoard_t gDashboard;
bool bExit=false;   

// Logs Ring Buffer
typedef struct {
    std::string txt;
    color_t     color;
} rbEntry_t;
rbEntry_t gRingBuffer[LOG_SIZE];
uint32_t gRBindex=0;
std::mutex gRBMutex;
std::string gExitMessage("");

/**
 * Get current date/time, format is YYYY-MM-DD.HH:mm:ss
 */ 
const std::string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
    return buf;
}

/**
 * 
 */
std::string& remove_leading_zeros(std::string& str)
{
    str.erase(0, min(str.find_first_not_of('0'), str.size()-1));
    return str;
}

/**
 * 
 */
std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}

/**
 * 
 */
int32_t sysCommand(std::string cmd, std::string & output) 
{
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

/**
 * 
 */
int32_t sysCommand(std::string cmd) 
{
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe){
        printf("sysCommand: [%s] error\n", cmd.c_str());
        return 1;
    } 
    pclose(pipe);
    return 0;
}

/**
 * 
 */
void copyStrToClipBoard(const std::string str)
{
    std::string cmd = "echo "+str+" | xclip -selection clipboard; echo "+str+" | xclip";
    sysCommand(cmd);
}

/**
 * 
 */
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

/**
 * 
 */
void addToRingBuffer(int32_t slotID, std::string newEntry, color_t color=NOCOLOR)
{
	if(gQuietMode)
		return;
		
    if(gDebugMode) {
        std::string txt =  std::string("Slot ") + std::to_string(slotID)  + std::string(": ") + newEntry;
        printf("%s\n", txt.c_str());
        return;
    }

    gRBMutex.lock();
    if(slotID>=0)
        gRingBuffer[gRBindex].txt   = currentDateTime() + std::string("   Board ") + std::to_string(slotID+1)  + std::string(": ") + newEntry;
    else
        gRingBuffer[gRBindex].txt   = currentDateTime() + std::string("            ") + newEntry;
    gRingBuffer[gRBindex].color = color;
    gRBindex = (gRBindex+1)%LOG_SIZE;   
    gRBMutex.unlock();
}

/**
 * 
 */
void printRB()
{
    gRBMutex.lock();
    // from index to end
    for(uint32_t i=gRBindex; i<LOG_SIZE; i++)
        printf("%-s\n\r", gRingBuffer[i].txt.c_str());
    
    // from start to index
    for(uint32_t i=0; i<gRBindex; i++)
        printf("%-s\n\r", gRingBuffer[i].txt.c_str());
    gRBMutex.unlock();
}

/**
 * 
 */
void saveInLogFile(const char* label, uint32_t value)
{
    std::string txt = std::string("echo ") + std::string(label) + std::string("=[") + to_string(value) + std::string("] >> data.log");
    system(txt.c_str());
}

/**
 * 
 */
void saveInLogFile(const char* label, std::string value)
{
    std::string txt = std::string("echo ") + std::string(label) + std::string("=[") + value + std::string("] >> data.log");
    system(txt.c_str());
}


/**
 * 
 */
void slotUpdate(void)
{
    gDashboard.slot[gDashboard.hlCell.slotID].slotOn = !gDashboard.slot[gDashboard.hlCell.slotID].slotOn;
    gDashboard.slot[gDashboard.hlCell.slotID].drmStatus = gDashboard.slot[gDashboard.hlCell.slotID].slotOn?IP_STATUS_STARTING:IP_STATUS_UNKNOWN;    
}
