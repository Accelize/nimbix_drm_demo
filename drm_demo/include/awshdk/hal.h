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

// FPGA Libs
#include <fpga_pci.h>
#include <fpga_mgmt.h>
#define FPGA_PROG_CMD std::string("sudo fpga-load-local-image -I agfi-0c35f2c467fcbf7f7 -S ")

// FPGA Design Defines
#define DRM_BASE_ADDRESS        0x10000
#define USER_IP_BASE_ADDRESS    0x00000

// Global variable
pci_bar_handle_t gHandle[MAX_NB_SLOTS];


/*
 * HAL Sanity Check
 */
int32_t halSanityCheck(void)
{
    return 0;
}

/*
 * DRMLib Read Callback Function
 */
int32_t my_read_drm(uint32_t slotID, uint32_t  addr, uint32_t * value)
{
    gDrmMutex.lock();
    if(fpga_pci_peek(gHandle[slotID], addr, value)) {
        std::cout << __FUNCTION__ << ": Unable to read from the fpga ! " << std::endl;
        gDrmMutex.unlock();
        return 1;
    }
    gDrmMutex.unlock();
    return 0;
}

/*
 * DRMLib Write Callback Function
 */
int32_t my_write_drm(uint32_t slotID, uint32_t  addr, uint32_t value)
{
    gDrmMutex.lock();
    if(fpga_pci_poke(gHandle[slotID], addr, value)) {
        std::cout << __FUNCTION__ << ": Unable to write to the fpga ! " << std::endl;
        gDrmMutex.unlock();
        return 1;
    }
    gDrmMutex.unlock();
    return 0;
}

/*
 * Program_FPGA
 */
int32_t progFPGA(uint32_t slotID)
{
    std::string cmd = FPGA_PROG_CMD + std::to_string(slotID);
    if(!gDebugMode)
        cmd += std::string(" > /dev/null");
    int32_t ret = system(cmd.c_str());
    return WEXITSTATUS(ret);
}

/**
 * Init FPGA
 */
int32_t initFPGA(uint32_t slotID)
{   
    /* Program FPGA from selected slot */
    addToRingBuffer(slotID, std::string("[INFO] Programming FPGA ..."));    
    if(progFPGA(slotID)) {
        addToRingBuffer(slotID, std::string("[ERROR] FPGA Programmation failed ..."), (char*)BOLDRED);
        gDashboard.slot[slotID].slotState = STATE_ERROR;
        return -1;
    }
    gHandle[slotID] = PCI_BAR_HANDLE_INIT;

    if(fpga_pci_init())
        return -1;
    if(fpga_pci_attach(slotID, FPGA_APP_PF, APP_PF_BAR0, 0, &gHandle[slotID]))
        return -1;
    return 0;
}

/**
 * Uninit FPGA
 */
void uninitFPGA(uint32_t slotID)
{
    addToRingBuffer(slotID, std::string("[INFO] Uninit FPGA ..."));
    gDashboard.slot[slotID].slotState = STATE_IDLE;
}

/*
 * sanity_check
 */
int sanity_check(uint32_t slotID) 
{
    int ret=0;
    struct fpga_mgmt_image_info info = {0};

    /* get local image description, contains status, vendor id, and device id. */
    if(fpga_mgmt_describe_local_image(slotID, &info,0)) {
        addToRingBuffer(slotID, std::string("[ERROR] Unable to get AFI information. Are you running as root?"), (char*)BOLDRED);    
        return 1;  
    }

    /* check to see if the slot is ready */
    if (info.status != FPGA_STATUS_LOADED) {
        addToRingBuffer(slotID, std::string("[ERROR] AFI is not in READY state !"), (char*)BOLDRED);
        return 1;
    }

   return ret;
}





