#
# Accelize DRM Reference Design Makefile
#
#  options:
#      None to target On-prem boards
#      AWS=1 to target AWS CSP
#      MOCK=1 to use mockup FPGA Application
AWS=0
#

EXENAME := helloworld_fpga
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
else
	CXXFLAGS += -Iinclude/xclhal2 $(XRT_INC_DIR)
	LDFLAGS += $(XRT_LIBDIR)
	LDLIBS += $(XRT_LIB) $(XLZDRM_LIB)
endif

ifeq ($(PROV),1)
	NLPROV := -DNLPROV
endif

test:
	echo "CURDIR=$(CURDIR)"

all : check_env $(EXENAME)

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
	@rm -f /usr/sbin/helloworld
	@ln -s /opt/accelize/helloworld_fpga/autorun.sh /usr/sbin/helloworld
	@$(eval USERS := $(wildcard /home/*))
	@-for userdir in $(USERS); do \
		cp helloworld_fpga.desktop $$userdir/Desktop/helloworld_fpga.desktop;\
	done

uninstall:
	@rm -rf /opt/accelize/helloworld_fpga
	@rm -f /usr/sbin/helloworld
	@$(eval USERS := $(wildcard /home/*))
	@-for userdir in $(USERS); do \
		rm -f $$userdir/Desktop/helloworld_fpga.desktop;\
	done
  
check_env:
ifeq ($(AWS),1)
ifndef SDK_DIR
	@$(error SDK_DIR is undefined. Try "source /opt/aws-fpga/sdk_setup.sh" to set the software environment)
endif
endif

.PHONY: all clean check_env install uninstall

.DEFAULT_GOAL := all
