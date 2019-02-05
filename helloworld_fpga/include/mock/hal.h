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
    return 0;
}

/*
 * DRMLib Write Callback Function
 */
int32_t my_write_drm(uint32_t slotID, uint32_t  addr, uint32_t value)
{
    return 0;
}

/*
 * User IP Register Read
 */
int32_t my_read_userip(uint32_t slotID, uint32_t  addr, uint32_t * value)
{   
    if(slotID==3)
        *value=0;
    else if(slotID==4)
        *value=1;
    else if(slotID==5)
        *value=2;
    else
        *value = 3;
    return 0;
}

/*
 * Program_FPGA
 */
int32_t progFPGA(uint32_t slotID)
{
    return 0;
} 


/**
 * Init FPGA
 */
int32_t initFPGA(uint32_t slotID)
{
    waddToRingBuffer(slotID, std::string("[INFO] Programming FPGA ..."));    
    waddToRingBuffer(slotID, std::string("[INFO] Initit FPGA successfull ..."));
    return 0;
}

/**
 * Uninit FPGA
 */
void uninitFPGA(uint32_t slotID)
{
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


/**
 * DRMLib quick MOCK
 */
typedef std::function<int/*errcode*/ (uint32_t /*register offset*/, uint32_t* /*returned data*/)> ReadRegisterCallback;
typedef std::function<int/*errcode*/ (uint32_t /*register offset*/, uint32_t /*data to write*/)> WriteRegisterCallback;
typedef std::function<void (const std::string&/*error message*/)> AsynchErrorCallback;
class DrmManager {
public:
	enum ParameterKey {
		SESSION_ID=0,
		METERING_DATA,
		NUM_ACTIVATORS
	};
	DrmManager() = delete; //!< No default constructor
	inline DrmManager(const std::string& conf_file_path, const std::string& cred_file_path, ReadRegisterCallback f_drm_read32, WriteRegisterCallback f_drm_write32, AsynchErrorCallback f_error_cb){};
	inline ~DrmManager() {}; //!< Destructor
	inline void activate(){};
	inline void deactivate(){};
	inline void get( ParameterKey keyId, std::string& json_string ){json_string=std::string("42");};
	inline void set( std::string& json_string ){};
};
