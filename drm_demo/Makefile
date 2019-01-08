#
# Accelize DRM Reference Design Makefile
#
#  options:
#      None to target On-prem boards
#      AWS=1 to target AWS CSP
#

EXENAME := drm_demo

CXX_EXT := cpp
CXX := g++

CXXFLAGS := -Wall -Werror -std=c++11

XRT_ROOT    := /opt/xilinx/xrt
XRT_INC_DIR := -I$(XRT_ROOT)/include
XRT_LIBDIR  := -L$(XRT_ROOT)/lib
XRT_LIB := -lxrt_core

SRCS := $(wildcard *.$(CXX_EXT))
OBJS := $(patsubst %.$(CXX_EXT), %.o, $(SRCS))

-include $(OBJS:.o=.d)

CXXFLAGS += -g -D_DEBUG $(XRT_INC_DIR)
LDFLAGS += $(XRT_LIBDIR)
LDLIBS += $(XRT_LIB) -laccelize_drm -lrt -pthread -lncurses

ifeq ($(AWS),1)
	LDLIBS += -lxrt_aws -lfpga_mgmt
	CSP := -DAWS
	CXXFLAGS += -I$(SDK_DIR)/userspace/include
endif

all : $(EXENAME) check_env

%.o: %.$(CXX_EXT)
	$(CXX) $(CXXFLAGS) $(MYCFLAGS) $(MYCXXFLAGS) $(CSP) -c $< -o $@
	$(CXX) $(CXXFLAGS) $(MYCFLAGS) $(MYCXXFLAGS) -c -MM $< -o $(patsubst %.o, %.d, $@)

$(EXENAME): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) 

clean:
	rm -rf *.o *.d $(EXENAME)
	
check_env:
ifeq ($(AWS),1)
ifndef SDK_DIR
    $(error SDK_DIR is undefined. Try "source sdk_setup.sh" to set the software environment)
endif
endif

.PHONY: all clean check_env

.DEFAULT_GOAL := all
