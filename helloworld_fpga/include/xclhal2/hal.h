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
#ifdef NOGUI
#include "common.h"
#define waddToRingBuffer(...) addToRingBuffer(__VA_ARGS__)
#else
#include "gui.h"
#endif

// Accelize DRMLib
#include "accelize/drm.h"
using namespace Accelize::DRM;
std::mutex gDrmMutex;

#include <xclhal2.h>
#define XCLBIN_PATH std::string("/opt/accelize/helloworld_fpga/bitstreams/u200/binary_container_1.xclbin")
#define FPGA_PROG_CMD std::string("/opt/xilinx/xrt/bin/xbutil program -p ")

// FPGA Design Defines
#define SDX_KERNEL_BASE_ADDR    0x1800000
#define DRM_BASE_ADDRESS        (SDX_KERNEL_BASE_ADDR+0x100)
#define USER_IP_BASE_ADDRESS    (SDX_KERNEL_BASE_ADDR+0x000)

// Global variable
xclDeviceHandle gHandle[MAX_NB_SLOTS];


int32_t getBitstreamDSA(std::string & DSAname)
{
    std::string vendor, boardid, name, major, minor;
    std::string vendor_cmd("tail -n 50 " + XCLBIN_PATH +" | grep -a \"platform vendor\" | cut -d'\"' -f2");
    std::string brdid_cmd("tail -n 50 " + XCLBIN_PATH +" | grep -a \"platform vendor\" | cut -d'\"' -f4");
    std::string name_cmd("tail -n 50 " + XCLBIN_PATH +" | grep -a \"platform vendor\" | cut -d'\"' -f6");
    std::string major_cmd("tail -n 50 " + XCLBIN_PATH +" | grep -a \"version major\" | cut -d'\"' -f2");
    std::string minor_cmd("tail -n 50 " + XCLBIN_PATH +" | grep -a \"version major\" | cut -d'\"' -f4");
    std::string out;
    
    if(sysCommand(vendor_cmd, vendor)) return 1;
    if(sysCommand(brdid_cmd, boardid)) return 1;
    if(sysCommand(name_cmd, name)) return 1;
    if(sysCommand(major_cmd, major)) return 1;
    if(sysCommand(minor_cmd, minor)) return 1;
    
    DSAname = vendor + std::string("_") + boardid + std::string("_") + name + std::string("_") + major + std::string("_") + minor;
    return 0;   
}

/*
 * HAL Sanity Check
 */
int32_t halSanityCheck(void)
{
    std::string bitstreamDSA, vmDSA;
    std::string getVmDSAcmd("/opt/xilinx/xrt/bin/xbutil list | tail -1 | cut -d' ' -f3");
    if(getBitstreamDSA(bitstreamDSA)) return 1;
    if(sysCommand(getVmDSAcmd, vmDSA)) return 1;
    
    if(vmDSA != bitstreamDSA) {
        printf("[ERROR] VM DSA [%s] differs from bitstream DSA [%s]\n", vmDSA.c_str(), bitstreamDSA.c_str());
        return 1;
    }
    return 0;
}

/*
 * DRMLib Read Callback Function
 */
int32_t my_read_drm(uint32_t slotID, uint32_t  addr, uint32_t * value)
{   
    gDrmMutex.lock();
    int ret = (int)xclRead(gHandle[slotID], XCL_ADDR_KERNEL_CTRL, DRM_BASE_ADDRESS+addr, value, 4);
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
int32_t my_write_drm(uint32_t slotID, uint32_t  addr, uint32_t value)
{
    gDrmMutex.lock();
    int ret = (int)xclWrite(gHandle[slotID], XCL_ADDR_KERNEL_CTRL, DRM_BASE_ADDRESS+addr, &value, 4);
    if(ret <= 0) {
        std::cout << __FUNCTION__ << ": Unable to write to the fpga ! ret=" << ret << std::endl;
        gDrmMutex.unlock();
        return 1;
    }
    gDrmMutex.unlock();
    return 0;
}

/*
 * User IP Register Read
 */
int32_t my_read_userip(uint32_t slotID, uint32_t  addr, uint32_t * value)
{   
    gDrmMutex.lock();
    int ret = (int)xclRead(gHandle[slotID], XCL_ADDR_KERNEL_CTRL, USER_IP_BASE_ADDRESS+addr, value, 4);
    if(ret <= 0) {
        std::cout << __FUNCTION__ << ": Unable to read from the fpga ! ret = " << ret << std::endl;
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
    std::string cmd = FPGA_PROG_CMD + XCLBIN_PATH + std::string(" -d ") + std::to_string(slotID);
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
    waddToRingBuffer(slotID, std::string("[INFO] Programming FPGA ..."));    
    if(progFPGA(slotID)) {
        waddToRingBuffer(slotID, std::string("[ERROR] FPGA Programmation failed ..."), ERROR_COLOR);
        gDashboard.slot[slotID].appState = STATE_ERROR;
        return -1;
    }
    if(xclProbe() < 1) {
        waddToRingBuffer(slotID, std::string("[ERROR] xclProbe failed ..."), ERROR_COLOR);
        gDashboard.slot[slotID].appState = STATE_ERROR;
        return -1;
    }
    waddToRingBuffer(slotID, std::string("[INFO] xclProbe success ..."));
    
    std::string logFileName = std::string("xcl_run_") + std::to_string(slotID) + std::string(".log");
    xclVerbosityLevel level = XCL_ERROR;
    gHandle[slotID] = xclOpen(slotID, logFileName.c_str(), level);
    if(gHandle[slotID] == NULL) {
        waddToRingBuffer(slotID, std::string("[ERROR] xclOpen failed ..."), ERROR_COLOR);
        gDashboard.slot[slotID].appState = STATE_ERROR;
        return -1;
    }
    waddToRingBuffer(slotID, std::string("[INFO] xclOpen success ..."));

    int32_t ret = xclLockDevice(gHandle[slotID]);
    if(ret != 0) {
        waddToRingBuffer(slotID, std::string("[ERROR] xclLockDevice failed ..."), ERROR_COLOR);
        gDashboard.slot[slotID].appState = STATE_ERROR;
        return ret;
    }
    waddToRingBuffer(slotID, std::string("[INFO] xclLockDevice success ..."));
    return 0;
}

/**
 * Uninit FPGA
 */
void uninitFPGA(uint32_t slotID)
{
    xclUnlockDevice(gHandle[slotID]);
    xclClose(gHandle[slotID]);
    waddToRingBuffer(slotID, std::string("[INFO] Uninit FPGA ..."));
    gDashboard.slot[slotID].appState = STATE_IDLE;
}

/*
 * sanity_check
 */
int sanity_check(uint32_t slot_id) 
{
   return 0;
}
