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

#ifdef MOCK
#include "mock/hal.h"
#else
#ifdef AWS
#include "awshdk/hal.h"
#else
#include "xclhal2/hal.h"
#endif
#endif

/**
 * Slot thread
 */
void SlotThread(uint32_t slotID)
{   
    uint32_t fsm = FSM_IDLE;
    DrmManager *pDrmManag;
    uint32_t savedSlotOnOff = gDashboard.slot[slotID].slotOn;
        
    while (!bExit) {    
        // Handle change of lic mode or disabling during exec       
        if( !gDashboard.slot[slotID].slotOn && savedSlotOnOff ) {
            if(fsm > FSM_INIT) {
                pDrmManag->deactivate();
                delete pDrmManag;
            }
            if(fsm > FSM_IDLE)
                uninitFPGA(slotID);
            waddToRingBuffer(slotID, std::string("[INFO] Ready ..."));
            fsm = FSM_IDLE; 
        }
        
        savedSlotOnOff = gDashboard.slot[slotID].slotOn;
        
        switch(fsm) {
            case FSM_IDLE:
            default:
                gDashboard.slot[slotID].appState=STATE_IDLE;
                gDashboard.slot[slotID].sessionID = std::string("");
                gDashboard.slot[slotID].usageUnits = std::string("");
                if(gDashboard.slot[slotID].slotOn) {
                    waddToRingBuffer(slotID, std::string("[INFO] Initializing FPGA ..."));   
                    if(initFPGA(slotID)) {
                        waddToRingBuffer(slotID, std::string("[ERROR] FPGA Initialization"), ERROR_COLOR);
                        gDashboard.slot[slotID].slotOn = !gDashboard.slot[slotID].slotOn;   
                        gDashboard.slot[gDashboard.hlCell.slotID].drmStatus = 0;
                        fsm = FSM_IDLE;
                    }
                    else
                        fsm = FSM_INIT;
                }
                break;
                
            case FSM_INIT:
                waddToRingBuffer(slotID, std::string("[INFO] Initializing Metering Session Manager ..."));                                   
                try {
                    pDrmManag = new DrmManager(
                        DRMDEMO_PATH + gDrmLibConfPath[gUserNameIndex] + std::string("conf.json"),
                        DRMDEMO_PATH + gDrmLibConfPath[gUserNameIndex] + std::string("cred.json"),
                        [&]( uint32_t  offset, uint32_t * value) { /*Read DRM register*/
                            return  my_read_drm(slotID, offset, value);
                        },
                        [&]( uint32_t  offset, uint32_t value) { /*Write DRM register*/
                            return my_write_drm(slotID, offset, value);
                        },
                        [&]( const  std::string & err_msg) {
                           std::cerr  << err_msg << std::endl;
                        }
                    );
                }
                catch (const std::exception& e) {                       
                    if(gUserNameIndex==LICENSE_MODE_NODELOCKED) {
                        gDashboard.slot[slotID].appState = STATE_NO_LIC;
                        waddToRingBuffer(slotID, std::string("[ERROR] No Local License File found"), WARNING_COLOR); 
                    }
                    else
                        waddToRingBuffer(slotID, std::string("[ERROR] Metering Session Manager Init Failed"), ERROR_COLOR);   
                    fsm = FSM_INIT;
                    break;
                }
                fsm = FSM_START_SESSION;
                break;
                
            case FSM_START_SESSION: 
                waddToRingBuffer(slotID, std::string("[INFO] Starting Session ..."));                
                try {
                    pDrmManag->activate();
                    fsm = FSM_RUNNING;
                    break;          
                }
                catch (const std::exception& e) {
                    if(gUserNameIndex==LICENSE_MODE_FLOATING) {
                        gDashboard.slot[slotID].appState = STATE_NO_SEAT;
                        waddToRingBuffer(slotID, std::string("[ERROR] No Seat Available"), WARNING_COLOR);   
                    }                   
                    if(gUserNameIndex==LICENSE_MODE_NODELOCKED) {
                        gDashboard.slot[slotID].appState = STATE_NO_LIC;
                        waddToRingBuffer(slotID, std::string("[ERROR] No Local License File found"), WARNING_COLOR); 
                    }                   
                    if(gUserNameIndex==LICENSE_MODE_METERED) {
                        gDashboard.slot[slotID].appState = STATE_ERROR;
                        waddToRingBuffer(slotID, std::string("[ERROR] Start Metering Session Failed"), ERROR_COLOR);  
                    }
                    delete pDrmManag;
                    fsm = FSM_INIT;
                }
                break;
                
            case FSM_RUNNING:
                try {
                    gDashboard.slot[slotID].appState=STATE_RUNNING;
                    uint32_t reg =0;
                    if(!gDashboard.slot[slotID].slotOn) {
                        pDrmManag->deactivate();
                        delete pDrmManag;
                        uninitFPGA(slotID);
                        waddToRingBuffer(slotID, std::string("[INFO] Ready ..."));
                        fsm = FSM_IDLE;
                    }
                    else {
                        waddToRingBuffer(slotID, std::string("[INFO] Reading DRM Status ..."));
                        my_read_userip(slotID, 0, &reg);
                        gDashboard.slot[slotID].drmStatus  = (reg&0x01)?IP_STATUS_ACTIVATED:IP_STATUS_LOCKED;
                        gDashboard.slot[slotID].sessionID  = pDrmManag->get<std::string>(session_id);
                        gDashboard.slot[slotID].usageUnits = pDrmManag->get<std::string>(metering_data);
                    }
                }
                catch (const std::exception& e) {
                    if(gUserNameIndex==LICENSE_MODE_FLOATING) {
                        gDashboard.slot[slotID].appState = STATE_NO_SEAT;
                        waddToRingBuffer(slotID, std::string("[ERROR] No Seat Available"), WARNING_COLOR);   
                    }                                       
                    if(gUserNameIndex==LICENSE_MODE_METERED) {
                        gDashboard.slot[slotID].appState = STATE_ERROR;
                        waddToRingBuffer(slotID, std::string("[ERROR] Metering Session Failed"), ERROR_COLOR);    
                    }
                }
                break;
            
        }
        sleep(1);
    }

    // Clean Exit
    if(fsm > FSM_INIT) {
        pDrmManag->deactivate();
        delete pDrmManag;
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
    std::cerr << "\t-u,--user          username [mary, fanny, nancy] (default=mary)" << std::endl;
    std::cerr << "\t-f,--fullscreen    full-screen mode (default=off)" << std::endl;
    std::cerr << "\t-d,--debug         enable debug mode (default=off)" << std::endl;
    std::cerr << ""  << std::endl; 
 }


/**
 *  Parse Command Line Arguments
 */
int parse_cmdline_arguments(int argc, char*argv[])
{
    std::string userName("mary@acme.com");
    const char* const short_opts = "n:u:v:dfh?";
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
                waddToRingBuffer(-1, std::string("[INFO] Nb Slots = ") + to_string(gDashboard.nbSlots));
                break;
            case 'u':
                userName = std::string(optarg);
                waddToRingBuffer(-1, std::string("[INFO] Username = ") + userName);
                break;
            case 'f':
                gFullScreenMode=true;
                waddToRingBuffer(-1, std::string("[INFO] Running in Fullscreen Mode"));
                break;            
            case 'v':
                setenv("ACCELIZE_DRM_VERBOSE", optarg, 1);
                waddToRingBuffer(-1, std::string("[INFO] Running in Verbose Mode Level ")+std::string(optarg));
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
        userName += std::string("@acme.com");
    
    setUserName(userName);
    
    if(!gDashboard.nbSlots || gDashboard.nbSlots>MAX_NB_SLOTS) {
        std::cout << "ERROR: Number of slots must be in range [1:" << MAX_NB_SLOTS << "]" << std::endl;
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
    int32_t retCode=0;
    try {
        std::string user = gAllowedUsers[gUserNameIndex];
        printf("Starting debugMode with user %s\n", user.c_str());
        DrmManager *pDrmManag;
        printf("[%s] Init FPGA ..\n", user.c_str());
        initFPGA(slotID);
        
        printf("[%s] Start DRM Manager ..\n", user.c_str());
        std::string conf_json_path = DRMDEMO_PATH + gDrmLibConfPath[gUserNameIndex] + std::string("conf.json");
        std::string cred_json_path = DRMDEMO_PATH + gDrmLibConfPath[gUserNameIndex] + std::string("cred.json");
        printf("[%s] \tconf.json path = [%s] ..\n", user.c_str(), conf_json_path.c_str());
        printf("[%s] \tcred.json path = [%s] ..\n", user.c_str(), cred_json_path.c_str());

        pDrmManag = new DrmManager(
            conf_json_path,
            cred_json_path,
            [&]( uint32_t  offset, uint32_t * value) { /*Read DRM register*/
                return  my_read_drm(slotID, offset, value);
            },
            [&]( uint32_t  offset, uint32_t value) { /*Write DRM register*/
                return my_write_drm(slotID, offset, value);
            },
            [&]( const  std::string & err_msg) {
               std::cerr  << err_msg << std::endl;
            }
        );
        sleep(2);
        
        printf("[%s] Get NL License Request ..\n", user.c_str());
        std::string lic_req_path = pDrmManag->get<std::string>(nodelocked_request_file);  
        
        if(lic_req_path != std::string("Not applicable")) {
            std::string lic_req_content("");
            printf("[%s] License Request File Path = %s \n", user.c_str(), lic_req_path.c_str());        
            sysCommand(std::string("cat ")+lic_req_path, lic_req_content);
            printf("[%s] License Request Content:\n%s\n", user.c_str(), lic_req_content.c_str());
        }
        
        printf("[%s] Start Session ..\n", user.c_str());
        pDrmManag->activate();
        sleep(1);
        
        printf("[%s] Reading IP Status ...\n", user.c_str());
        uint32_t reg =0;
        my_read_userip(slotID, 0, &reg);
        printf("[%s] IP Status = %d\n", user.c_str(), reg);
        if(reg != 3)
            retCode++;      
        
        std::string sessionID("");
        std::string usageUnits("");
        sessionID  = pDrmManag->get<std::string>(session_id);        
        printf("[%s] Session ID = %s\n", user.c_str(), sessionID.c_str());
        
        //for(uint32_t i=0; i<30; i++) {
        //    usageUnits = pDrmManag->get<std::string>(metering_data);
        //    printf("[%s] Usage Units (%d secs)= %s\n", user.c_str(), i+1, remove_leading_zeros(usageUnits).c_str());
        //    sleep(1);
        //}
        
        printf("[%s] Stop Session ..\n", user.c_str());
        pDrmManag->deactivate();
        
        sleep(10);
        
        usageUnits = pDrmManag->get<std::string>(metering_data);
        printf("[%s] Usage Units = %s\n", user.c_str(), remove_leading_zeros(usageUnits).c_str());
        if(remove_leading_zeros(usageUnits) == std::string("0"))
            retCode++;
        
        printf("[%s] Uninit FPGA ..\n", user.c_str());
        uninitFPGA(slotID); 
    }
    catch (const std::exception& e) {
        std::cout << "Exception caught:" << std::endl;
        std::cout << e.what() << std::endl;
        return -1;
    }
    return retCode;
}


/**
 * 
 */
int main(int argc, char **argv) 
{       
    gDashboard.nbSlots = MAX_NB_SLOTS;
    
    /* Retrieve input arguments */
    if(parse_cmdline_arguments(argc, argv))
        return 1;
        
    /* HAL Sanity Check */
    if(halSanityCheck())
        return 1;
        
    if(gDebugMode) {
        return debugMode(0);
    }
    
    if(gui_init())
        return 1; 
    
    // Start slot threads
    thread slot_threads[MAX_NB_SLOTS];
    for(uint32_t i=0; i<gDashboard.nbSlots; i++)
        slot_threads[i] = thread(SlotThread, i);
    
    // Start user inputs thread
    thread inputevents_thread(InputEventsThread);
    
    // Join threads
    inputevents_thread.join();
    for(uint32_t i=0; i<gDashboard.nbSlots; i++)
        slot_threads[i].join();
    
    gui_uninit();
    
    std::cout << gExitMessage << std::endl;
    return 0;
}

