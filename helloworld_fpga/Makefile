#
# Accelize DRM Reference Design Makefile
#
#  options:
#      None to target On-prem boards
#      AWS=1 to target AWS CSP
#      MOCK=1 to use mockup FPGA Application
AWS=0
#

EXENAME := helloWorldFPGA
CURDIR := $(shell pwd)

CXX_EXT := cpp
CXX := g++
CXXFLAGS := -Wall -Werror -std=c++11


# Xilinx Runtime (XRT)
XRT_ROOT    := /opt/xilinx/xrt
XRT_INC_DIR := -I$(XRT_ROOT)/include
XRT_LIBDIR  := -L$(XRT_ROOT)/lib
XRT_LIB := -lxrt_core

# Accelize DRMLib
XLZDRM_LIB = -laccelize_drm

SRCS := $(wildcard *.$(CXX_EXT))
OBJS := $(patsubst %.$(CXX_EXT), %.o, $(SRCS))
-include $(OBJS:.o=.d)

CXXFLAGS += -Iinclude -I$(SDK_DIR)/userspace/include
LDFLAGS += 
LDLIBS += -lrt -pthread -lncurses
RUNSCRIPT=run.sh.op

# Debug Mode
ifeq ($(DEBUG),1)
CXXFLAGS += -g -DDEBUG
endif

# Fullscreen Mode
ifeq ($(FULL),1)
CXXFLAGS += -DFULL
endif

ifeq ($(MOCK),1)
	CSP := -DMOCK
	CXXFLAGS += -Iinclude/mock
else ifeq ($(AWS),1)
	CXXFLAGS +=  -Iinclude/awshdk
	LDFLAGS += $(XRT_LIBDIR)
	LDLIBS += -lxrt_aws -lfpga_mgmt $(XLZDRM_LIB)
	CSP := -DAWS	
	RUNSCRIPT := run.sh.aws
else ifeq ($(HUAWEI),1)
	CXXFLAGS += -I${XILINX_SDX}/runtime/include/1_2
	LDFLAGS += -L${XILINX_SDX}/runtime/lib/x86_64
	LDFLAGS += -L${XILINX_SDX}/lib/lnx64.o/ -lxilinxopencl -lrt -pthread -lstdc++ -lboost_filesystem

	XRT_ROOT    := /home/huaweicloud-fpga/fp1/software/userspace/sdaccel/driver/xclgemhal/driver
	XRT_INC_DIR := -I$(XRT_ROOT)/include
	XRT_LIBDIR  := -L/home/huaweicloud-fpga/fp1/software/userspace/sdaccel/lib/runtime/platforms/hal/driver
	XRT_LIB     := -lxclgemdrv

	CXXFLAGS +=  -Iinclude -I$(SDK_DIR)/userspace/include $(XRT_INC_DIR)
	LDFLAGS += $(XRT_LIBDIR)
	LDLIBS += $(XRT_LIB) $(XLZDRM_LIB) 
	CSP := -DHUAWEI
	RUNSCRIPT := run.sh.huawei
else
	CXXFLAGS += -Iinclude/xclhal2 $(XRT_INC_DIR)
	LDFLAGS += $(XRT_LIBDIR)
	LDLIBS += $(XRT_LIB) $(XLZDRM_LIB)
	RUNSCRIPT := run.sh.op
endif

ifeq ($(PROV),1)
	NLPROV := -DNLPROV
endif

run_script:
	@echo "RUNSCRIPT=$(RUNSCRIPT)"
	@cp $(RUNSCRIPT) run.sh

all : check_env $(EXENAME) run_script

%.o: %.$(CXX_EXT)
	$(CXX) $(CXXFLAGS) $(MYCFLAGS) $(MYCXXFLAGS) $(CSP) $(NLPROV) -c $< -o $@
	$(CXX) $(CXXFLAGS) $(MYCFLAGS) $(MYCXXFLAGS) -c -MM $< -o $(patsubst %.o, %.d, $@)

$(EXENAME): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) 

clean:
	@rm -rf *.o *.d *.log $(EXENAME)
	
install: uninstall
	@mkdir -p /opt/accelize
	@cp -rf $(CURDIR) /opt/accelize/helloworld_fpga
	@chmod -R 755 /opt/accelize
	@rm -f /usr/sbin/helloWorldFPGA
	@ln -s /opt/accelize/helloworld_fpga/autorun.sh /usr/sbin/helloWorldFPGA
	@$(eval USERS := $(wildcard /home/*))
	@-for userdir in $(USERS); do \
		cp helloWorldFPGA.desktop $$userdir/Desktop/helloWorldFPGA.desktop;\
	done

uninstall:
	@rm -rf /opt/accelize/helloworld_fpga
	@rm -f /usr/sbin/helloWorldFPGA
	@$(eval USERS := $(wildcard /home/*))
	@-for userdir in $(USERS); do \
		rm -f $$userdir/Desktop/helloWorldFPGA.desktop;\
	done
  
check_env:
ifeq ($(AWS),1)
ifndef SDK_DIR
	@$(error SDK_DIR is undefined. Try "source /opt/aws-fpga/sdk_setup.sh" to set the software environment)
endif
endif
ifeq ($(HUAWEI),1)
ifndef FPGA_TOOL_DIR
    @$(error FP1 environment not configured. Try "source /home/huaweicloud-fpga/fp1/setup.sh" to setup the FP1 environment)
endif
endif

.PHONY: all clean check_env install uninstall

.DEFAULT_GOAL := all
