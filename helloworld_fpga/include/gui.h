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

#include "common.h"
#include <X11/Xlib.h>

#ifdef FULL
    #define CELL_HEIGHT                 5
#else                                   
    #define CELL_HEIGHT                 3
#endif 

#define ENABLE_LOGS                     1                                 

#define SESSION_ID_STRG_SIZE            16
#define APPLOGO_HEIGHT                  4
#define CELL_WIDTH                      (SESSION_ID_STRG_SIZE+2)
#define DBCOL_NB_ROWS                   7
#define DASHBOARD_XOFFSET               1
#define DASHBOARD_YOFFSET               1
#define DASHBOARD_APPLOGO_XOFFSET       0
#define DASHBOARD_APPLOGO_YOFFSET       DASHBOARD_YOFFSET
#define DASHBOARD_SPINNER_XOFFSET       DASHBOARD_XOFFSET
#define DASHBOARD_SPINNER_YOFFSET       (DASHBOARD_YOFFSET+APPLOGO_HEIGHT+1)
#define DASHBOARD_UNAME_XOFFSET         DASHBOARD_XOFFSET
#define DASHBOARD_UNAME_YOFFSET         (DASHBOARD_SPINNER_YOFFSET+(CELL_HEIGHT-1))
#define DASHBOARD_CELL_LABEL_XOFFSET    DASHBOARD_XOFFSET
#define DASHBOARD_CELL_LABEL_YOFFSET    (DASHBOARD_UNAME_YOFFSET+(CELL_HEIGHT-1))
#define DASHBOARD_DYNCELLS_XOFFSET      (DASHBOARD_XOFFSET+CELL_WIDTH-1)
#define DASHBOARD_DYNCELLS_YOFFSET      (DASHBOARD_CELL_LABEL_YOFFSET)
#define DASHBOARD_SESSID_YOFFSET        (DASHBOARD_CELL_LABEL_YOFFSET+8)
#define RBLOG_WINSIZE                   (LOG_SIZE)
#define DASHBOARD_RBLOGS_XOFFSET        DASHBOARD_XOFFSET+1
#define DASHBOARD_RBLOGS_YOFFSET        (DASHBOARD_CELL_LABEL_YOFFSET+(DBCOL_NB_ROWS*(CELL_HEIGHT-1))+1)
#define DASHBOARD_RBLOGS_WIDTH          (MAX_NB_SLOTS*(CELL_WIDTH-1))
#define TERMINAL_MIN_WIDTH              ((MAX_NB_SLOTS+1)*(CELL_WIDTH-1)+DASHBOARD_XOFFSET+1)
#define TERMINAL_MIN_HEIGHT             (DASHBOARD_RBLOGS_YOFFSET+RBLOG_WINSIZE+DASHBOARD_YOFFSET+1)
#define MOUSE_EXIT_YMAX                 DASHBOARD_SPINNER_YOFFSET
#define MOUSE_UNAME_YMIN                DASHBOARD_UNAME_YOFFSET
#define MOUSE_UNAME_YMAX                DASHBOARD_CELL_LABEL_YOFFSET
#define BLANK_CELL                      "----"
                                        
typedef struct {
    WINDOW* parent;
    WINDOW* win;
    WINDOW* slotNb;
    WINDOW* appOnOff;
    WINDOW* drmStatus;
    WINDOW* appStatus;
    WINDOW* sessionID;
    WINDOW* usageUnits;
    WINDOW* appOutput; 
} dbCol_t;

WINDOW *gWinBkgnd;
WINDOW* gUnameWin;
WINDOW* gSpinnerWin;
WINDOW* gRBlogsWin;
WINDOW* gRBlogsBordersWin;
dbCol_t gColHeader;
dbCol_t gDBcols[MAX_NB_SLOTS];

std::string gSpinnerArray[]={   "Running...........", 
                                "unning...........R", 
                                "nning...........Ru", 
                                "ning...........Run",
                                "ing...........Runn", 
                                "ng...........Runni", 
                                "g...........Runnin", 
                                "...........Running",
                                "..........Running.",
                                ".........Running..",
                                "........Running...",
                                ".......Running....",
                                "......Running.....",
                                ".....Running......",
                                "....Running.......",
                                "...Running........",
                                "..Running.........",
                                ".Running..........",
                                };

uint32_t gSpinnerIdx=0;
std::string  AppLogoSmall("\
    _   ___ ___ ___ _    ___ _______   _  _     _ _   __      __       _    _   ___ ___  ___   _   \n\
   /_\\ / __/ __| __| |  |_ _|_  / __| | || |___| | |__\\ \\    / /__ _ _| |__| | | __| _ \\/ __| /_\\ \n\
  / _ \\ (_| (__| _|| |__ | | / /| _|  | __ / -_) | / _ \\ \\/\\/ / _ \\ '_| / _` | | _||  _/ (_ |/ _ \\ \n\
 /_/ \\_\\___\\___|___|____|___/___|___| |_||_\\___|_|_\\___/\\_/\\_/\\___/_| |_\\__,_| |_| |_|  \\___/_/ \\_\\\n"
);

/**
 * 
 */
bool checkTerminalResize(uint32_t initWidth, uint32_t initHeight)
{
    uint32_t curWidth=0, curHeight=0;
    getmaxyx(stdscr, curHeight, curWidth);
    
    // Detect Terminal resizing
    if(curWidth!=initWidth || curHeight!=initHeight) {
        gExitMessage = std::string("\n\nThis application does not support dynamic resizing\nPlease relaunch application after changing terminal size\n\n");
        return true;
    }
    return false;
}

/**
 * 
 */
void switchColorAttr(WINDOW* win, color_t color, bool enable)
{
    init_pair(NOCOLOR,          COLOR_WHITE,  COLOR_BLACK);
    init_pair(HIGHLIT_COLOR,    COLOR_BLACK,  COLOR_WHITE);
    init_pair(BOLDWHITE_COLOR,  COLOR_WHITE,  COLOR_BLACK);
    init_pair(ERROR_COLOR,      COLOR_RED,    COLOR_BLACK);
    init_pair(SUCESS_COLOR,     COLOR_GREEN,  COLOR_BLACK);
    init_pair(WARNING_COLOR,    COLOR_YELLOW, COLOR_BLACK);
    init_pair(LOCKED_COLOR,     COLOR_WHITE,  COLOR_RED  );
    init_pair(UNLOCKED_COLOR,   COLOR_WHITE,  COLOR_GREEN);

    if(enable) {
        wattron(win, COLOR_PAIR(color));
        if(color>NOCOLOR)
            wattron(win, A_BOLD);
    }
    else {
        wattroff(win, COLOR_PAIR(color));
        wattroff(win, A_BOLD);
        wattron(win, COLOR_PAIR(NOCOLOR));
    }
}

/**
 * 
 */
int32_t gui_init()
{        
    initscr();              // Initialize the window
    
    // Verify Terminal size
    if(COLS<TERMINAL_MIN_WIDTH || LINES<TERMINAL_MIN_HEIGHT) {  
        endwin();           // Restore normal terminal behavior 
        std::string curTermSize = to_string(COLS) + std::string("x") + to_string(LINES);
        std::string minTermSize = to_string(TERMINAL_MIN_WIDTH) + std::string("x") + to_string(TERMINAL_MIN_HEIGHT);     
        std::cout << "\n\nTerminal size (" << curTermSize << ") is insufficient to run the demo" << std::endl;
        std::cout << "Minimal requierement is " + minTermSize + std::string("\n") << std::endl;
        return true;
    }
    
    cbreak();               // Line buffering disabled
    noecho();               // Don't echo any keypresses
    curs_set(FALSE);        // Don't display a cursor
    keypad(stdscr, TRUE);   // Catch Keyboard inputs
    nodelay(stdscr, TRUE);
    start_color();
    return 0;
}

/**
 * 
 */
void printCentered(WINDOW* win, std::string txt, color_t color=NOCOLOR)
{
    switchColorAttr(win, color, true);
    uint32_t curY=0, curX=0, txtLength=strlen(txt.c_str());
    getmaxyx(win, curY, curX);    
    char emptyTxt[TERMINAL_MIN_WIDTH];
    memset(emptyTxt, ' ', sizeof(emptyTxt));
    for(uint32_t i=1; i<(curX-1); i++)
        mvwprintw(win, i, 0, emptyTxt);
    mvwprintw(win, (curY/2), (curX/2)-(txtLength/2), txt.c_str());
    switchColorAttr(win, color, false);
}

/**
 * 
 */
dbCol_t createDBcol(WINDOW* parent, std::string slotID, std::string appOnOff, std::string drmStatus,
                    std::string appStatus, std::string sessionID, std::string usageUnits, std::string appOutput,
                    uint32_t startX, uint32_t startY, color_t color=NOCOLOR)
{    
    dbCol_t dbWin;
    dbWin.parent     = parent;
    dbWin.win        = derwin(parent,    DBCOL_NB_ROWS*(CELL_HEIGHT-1)+1, CELL_WIDTH, startY, startX);
    dbWin.slotNb     = derwin(dbWin.win, CELL_HEIGHT, CELL_WIDTH, 0*(CELL_HEIGHT-1), 0);
    dbWin.appOnOff   = derwin(dbWin.win, CELL_HEIGHT, CELL_WIDTH, 1*(CELL_HEIGHT-1), 0);
    dbWin.drmStatus  = derwin(dbWin.win, CELL_HEIGHT, CELL_WIDTH, 2*(CELL_HEIGHT-1), 0);
    dbWin.appStatus  = derwin(dbWin.win, CELL_HEIGHT, CELL_WIDTH, 3*(CELL_HEIGHT-1), 0);
    dbWin.sessionID  = derwin(dbWin.win, CELL_HEIGHT, CELL_WIDTH, 4*(CELL_HEIGHT-1), 0);
    dbWin.usageUnits = derwin(dbWin.win, CELL_HEIGHT, CELL_WIDTH, 5*(CELL_HEIGHT-1), 0);
    dbWin.appOutput  = derwin(dbWin.win, CELL_HEIGHT, CELL_WIDTH, 6*(CELL_HEIGHT-1), 0);
    
    printCentered(dbWin.slotNb, slotID, BOLDWHITE_COLOR);
    printCentered(dbWin.appOnOff, appOnOff, color);
    printCentered(dbWin.drmStatus, drmStatus, color);
    printCentered(dbWin.appStatus, appStatus, color);
    printCentered(dbWin.sessionID, sessionID, color);
    printCentered(dbWin.usageUnits, usageUnits, color);
    printCentered(dbWin.appOutput, appOutput, color);
    
    wborder(dbWin.win      , '|', '|', '-', '-', '+', '+', '+', '+');
    wborder(dbWin.slotNb   , '|', '|', '-', '-', '+', '+', '+', '+');
    wborder(dbWin.appOnOff , '|', '|', '-', '-', '+', '+', '+', '+');
    wborder(dbWin.drmStatus, '|', '|', '-', '-', '+', '+', '+', '+');
    wborder(dbWin.appStatus, '|', '|', '-', '-', '+', '+', '+', '+');
    wborder(dbWin.sessionID, '|', '|', '-', '-', '+', '+', '+', '+');
    wborder(dbWin.usageUnits, '|', '|', '-', '-', '+', '+', '+', '+'); 
    wborder(dbWin.appOutput, '|', '|', '-', '-', '+', '+', '+', '+');        
    return dbWin;
}


/**
 * 
 */
void delete_winbkgnd(WINDOW* win)
{
    delwin(gUnameWin);
    delwin(gSpinnerWin);
#if ENABLE_LOGS
    delwin(gRBlogsWin);
    delwin(gRBlogsBordersWin);  
#endif
    delwin(gColHeader.win);    
    for(unsigned int i=0; i<MAX_NB_SLOTS; i++)
        delwin(gDBcols[i].win);
    delwin(win);
}

/**
 * 
 */
WINDOW *create_winbkgnd(int height, int width, int starty, int startx)
{
    gWinBkgnd = newwin(0, 0, starty, startx);
    
    // Display Application Header
    mvwprintw(gWinBkgnd, DASHBOARD_APPLOGO_YOFFSET, DASHBOARD_APPLOGO_XOFFSET, AppLogoSmall.c_str());
    
    // Spinner
    gSpinnerWin = derwin(gWinBkgnd, CELL_HEIGHT, CELL_WIDTH, 
                            DASHBOARD_SPINNER_YOFFSET, DASHBOARD_SPINNER_XOFFSET);
    
    // Create username window
    gUnameWin = derwin(gWinBkgnd, CELL_HEIGHT, (MAX_NB_SLOTS+1)*(CELL_WIDTH-1)+1, 
                            DASHBOARD_UNAME_YOFFSET, DASHBOARD_UNAME_XOFFSET);
    printCentered(gUnameWin, gAllowedUsers[gUserNameIndex] + std::string(" - ") + gLicenseModeStr[gUserNameIndex], BOLDWHITE_COLOR);
    wborder(gUnameWin      , '|', '|', '-', '-', '+', '+', '+', '+');
    
    // create static label column 
    gColHeader = createDBcol(gWinBkgnd, "FPGA Board", "App On/Off", "DRM Status", "App Status", "Session ID", "Usage Units", "App Output",
                            DASHBOARD_CELL_LABEL_XOFFSET, DASHBOARD_CELL_LABEL_YOFFSET, BOLDWHITE_COLOR);
    
    // Create dynamic columns Window
    // - ENABLED ONES
    for(unsigned int i=0; i<gDashboard.nbSlots; i++) {
        uint32_t startX = i*(CELL_WIDTH-1) + DASHBOARD_DYNCELLS_XOFFSET;
        uint32_t startY = DASHBOARD_DYNCELLS_YOFFSET;
        std::string appOnOff   = gAppStatusStr[gDashboard.slot[i].slotOn];
        std::string drmStatus  = gDrmStatusStr[gDashboard.slot[i].drmStatus];
        std::string appStatus  = gAppStateStr[gDashboard.slot[i].appState];
        std::string sessionID  = std::string("");
        std::string usageUnits = std::string(""); 
        std::string appOutput = std::string("");        
        gDBcols[i] = createDBcol(gWinBkgnd, std::to_string(i+1), appOnOff, 
                                drmStatus, appStatus, sessionID, usageUnits, 
                                appOutput, startX, startY);
        wrefresh(gDBcols[i].win);
    }
    // - DISABLED ONES
    for(unsigned int i=gDashboard.nbSlots; i<MAX_NB_SLOTS; i++) {
        uint32_t startX = i*(CELL_WIDTH-1) + DASHBOARD_DYNCELLS_XOFFSET;
        uint32_t startY = DASHBOARD_DYNCELLS_YOFFSET;
        gDBcols[i] = createDBcol(gWinBkgnd, BLANK_CELL, BLANK_CELL, 
                                BLANK_CELL, BLANK_CELL, BLANK_CELL, BLANK_CELL,
                                BLANK_CELL, startX, startY);
        wrefresh(gDBcols[i].win);
    }
    //Higlight first cell
    printCentered(gDBcols[0].appOnOff, gAppStatusStr[gDashboard.slot[0].slotOn], HIGHLIT_COLOR);
    wborder(gDBcols[0].appOnOff      , '|', '|', '-', '-', '+', '+', '+', '+');
    
#if ENABLE_LOGS
    // Create RingBuffer Logs Window
    gRBlogsBordersWin = derwin(gWinBkgnd, RBLOG_WINSIZE+2, (MAX_NB_SLOTS+1)*(CELL_WIDTH-1)+1, 
                            DASHBOARD_RBLOGS_YOFFSET-1, DASHBOARD_XOFFSET);
    wborder(gRBlogsBordersWin      , '|', '|', '-', '-', '+', '+', '+', '+');
    gRBlogsWin = derwin(gWinBkgnd, RBLOG_WINSIZE, (MAX_NB_SLOTS+1)*(CELL_WIDTH-1)+1-2, 
                            DASHBOARD_RBLOGS_YOFFSET, DASHBOARD_RBLOGS_XOFFSET);
 #endif
                        
    wrefresh(gWinBkgnd);
    return gWinBkgnd; 
}

/**
 * 
 */
#if ENABLE_LOGS
void gui_rbupdate()
{
    uint32_t line=0;
    gRBMutex.lock();
    werase(gRBlogsWin);
    // from index to end
    for(uint32_t i=gRBindex; i<LOG_SIZE; i++) {
        switchColorAttr(gRBlogsWin, gRingBuffer[i].color, true);
        std::string logEntry = gRingBuffer[i].txt; 
        mvwprintw(gRBlogsWin, line++, 2, rtrim(gRingBuffer[i].txt).c_str());
        switchColorAttr(gRBlogsWin, gRingBuffer[i].color, false);
    }
    // from start to index
    for(uint32_t i=0; i<gRBindex; i++) {
        switchColorAttr(gRBlogsWin, gRingBuffer[i].color, true);
        mvwprintw(gRBlogsWin, line++, 2, rtrim(gRingBuffer[i].txt).c_str());
        switchColorAttr(gRBlogsWin, gRingBuffer[i].color, false);
    }
    wrefresh(gRBlogsWin); 
    gRBMutex.unlock();
}
#endif


/**
 * 
 */
void gui_periodic_update()
{
    // Spinner Update 
    gSpinnerIdx = (gSpinnerIdx+1)%(sizeof(gSpinnerArray)/sizeof(gSpinnerArray[0]));
    mvwprintw(gSpinnerWin, 0, 0, gSpinnerArray[gSpinnerIdx].c_str());      
    wrefresh(gSpinnerWin);
    
    gui_rbupdate();
    
    // Slot infos
    for(unsigned int slotID=0; slotID<gDashboard.nbSlots; slotID++) {
        WINDOW* drmStatusWin = gDBcols[slotID].drmStatus;
        std::string drmStatusStr = gDrmStatusStr[gDashboard.slot[slotID].drmStatus];
        color_t drmStatusCol = gDrmStatusColor[gDashboard.slot[slotID].drmStatus];
        printCentered(drmStatusWin, drmStatusStr, drmStatusCol);
        wborder(drmStatusWin , '|', '|', '-', '-', '+', '+', '+', '+');
        wrefresh(drmStatusWin);
        
        WINDOW* appStatusWin = gDBcols[slotID].appStatus;
        std::string appStatusStr = gAppStateStr[gDashboard.slot[slotID].appState];
        printCentered(appStatusWin, appStatusStr);
        wborder(appStatusWin , '|', '|', '-', '-', '+', '+', '+', '+');
        wrefresh(appStatusWin);
    
        WINDOW* sessionIDWin = gDBcols[slotID].sessionID;
        printCentered(sessionIDWin, gDashboard.slot[slotID].sessionID);
        wborder(sessionIDWin , '|', '|', '-', '-', '+', '+', '+', '+');
        wrefresh(sessionIDWin);
        
        WINDOW* usageUnitsWin = gDBcols[slotID].usageUnits;
        printCentered(usageUnitsWin, remove_leading_zeros(gDashboard.slot[slotID].usageUnits));
        wborder(usageUnitsWin , '|', '|', '-', '-', '+', '+', '+', '+');
        wrefresh(usageUnitsWin);
        
        WINDOW* appOutputWin = gDBcols[slotID].appOutput;
        std::string appOutputStr("");
        if(gDashboard.slot[slotID].drmStatus==IP_STATUS_ACTIVATED)
            appOutputStr = std::string("Hello from ") + to_string(slotID) + std::string(" !");     
        printCentered(appOutputWin, appOutputStr);
        wborder(appOutputWin , '|', '|', '-', '-', '+', '+', '+', '+');
        wrefresh(appOutputWin);
    }
}

/**
 * 
 */
void gui_hlupdate(uint32_t prevID, uint32_t newID)
{
    WINDOW* oldwin = gDBcols[prevID].appOnOff;
    WINDOW* newwin = gDBcols[newID].appOnOff;
    printCentered(oldwin, gAppStatusStr[gDashboard.slot[prevID].slotOn], NOCOLOR);
    printCentered(newwin, gAppStatusStr[gDashboard.slot[newID].slotOn], HIGHLIT_COLOR);
    wborder(oldwin , '|', '|', '-', '-', '+', '+', '+', '+');
    wborder(newwin , '|', '|', '-', '-', '+', '+', '+', '+');
    wrefresh(oldwin);
    wrefresh(newwin);
}


/**
 * 
 */
void gui_slotupdate(uint32_t slotID)
{
    slotUpdate();
    WINDOW* win = gDBcols[slotID].appOnOff;
    wclear(win);
    std::string appOnOffStr  = gAppStatusStr[gDashboard.slot[slotID].slotOn];
    printCentered(win, appOnOffStr, HIGHLIT_COLOR);
    wborder(win , '|', '|', '-', '-', '+', '+', '+', '+');
    wrefresh(win);
}

/**
 * 
 */
void gui_unameupdate()
{
    wclear(gUnameWin);
    printCentered(gUnameWin, gAllowedUsers[gUserNameIndex] + std::string(" - ") + 
                    gLicenseModeStr[gUserNameIndex], BOLDWHITE_COLOR);
    wborder(gUnameWin      , '|', '|', '-', '-', '+', '+', '+', '+');
    wrefresh(gUnameWin);
}

/**
 * 
 */
void mouseEventsCallback(uint32_t x, uint32_t y) 
{   
    uint32_t prevSlotId     = gDashboard.hlCell.slotID;
    
    // Exit hidden feature
    if(y<=MOUSE_EXIT_YMAX) 
        bExit=true;
        
    // Switch user hidden feature (only when all slots are OFF)
    if(y>=MOUSE_UNAME_YMIN && y<=MOUSE_UNAME_YMAX) {
        for(uint32_t i=0; i<gDashboard.nbSlots; i++)
            if(gDashboard.slot[i].slotOn)
                return; 
        if(gUserNameIndex==userIdx) // switch disabled when user account used
            return; 
        gUserNameIndex+=1;
        if(gUserNameIndex==userIdx) // Skip user account in demo mode
            gUserNameIndex=nancyIdx;  
        gui_unameupdate();
        return;
    }
    
    // Update slot ID
    if(x < (DASHBOARD_DYNCELLS_XOFFSET+CELL_WIDTH))
        gDashboard.hlCell.slotID = 0;
    else if(x> (DASHBOARD_DYNCELLS_XOFFSET + gDashboard.nbSlots*(CELL_WIDTH)))
        gDashboard.hlCell.slotID = gDashboard.nbSlots-1;
    else
        gDashboard.hlCell.slotID = (x-(DASHBOARD_DYNCELLS_XOFFSET)) / (CELL_WIDTH);
        
    // No action when clicking on Session ID (to copy into clipboard)
    if( y>=DASHBOARD_SESSID_YOFFSET && y<(DASHBOARD_SESSID_YOFFSET+3)) {
		copyStrToClipBoard(gDashboard.slot[gDashboard.hlCell.slotID].sessionID);
		return;    
	}
        
    // GUI Update
    if( gDashboard.hlCell.slotID == prevSlotId)
        gui_slotupdate(gDashboard.hlCell.slotID);
    else
        gui_hlupdate(prevSlotId, gDashboard.hlCell.slotID);        
    return;
}

/**
 * 
 */
void InputEventsThread()
{   
    WINDOW *my_win;
    uint32_t prevID = 0;
    uint32_t initWidth=COLS, initHeight=LINES;
    
    /* Get all the mouse events */
    MEVENT mevent;
    mousemask(BUTTON1_CLICKED|REPORT_MOUSE_POSITION, NULL);
    
    refresh();
    my_win = create_winbkgnd(0, 0, 0, 0);
    
    while(!bExit) {   
        if(checkTerminalResize(initWidth, initHeight)) {
            bExit=true;
            continue;
        }
        switch(getch()) {       
            case KEY_MOUSE:
                if(getmouse(&mevent)==OK)
                    mouseEventsCallback(mevent.x, mevent.y);           
                break; 
            case KEY_LEFT:
                prevID = gDashboard.hlCell.slotID;
                if(gDashboard.hlCell.slotID == 0)
                    gDashboard.hlCell.slotID = gDashboard.nbSlots-1;
                else
                    gDashboard.hlCell.slotID -= 1;
                gui_hlupdate(prevID, gDashboard.hlCell.slotID);
                break;                
            case KEY_RIGHT:
                prevID = gDashboard.hlCell.slotID;
                if(gDashboard.hlCell.slotID == (gDashboard.nbSlots-1))
                    gDashboard.hlCell.slotID = 0;
                else
                    gDashboard.hlCell.slotID += 1;
                gui_hlupdate(prevID, gDashboard.hlCell.slotID);
                break;
            case ' ':
            case '\n':
                gui_slotupdate(gDashboard.hlCell.slotID);
                break;  
            case 'q':
            case 'Q':
                bExit=true;
                break;
            case ERR:
            default:
                break;
        }
        gui_periodic_update();
        usleep(100000);
    }
    delete_winbkgnd(my_win);
}

/**
 * 
 */
void gui_uninit()
{
    endwin(); // Restore normal terminal behavior
}


