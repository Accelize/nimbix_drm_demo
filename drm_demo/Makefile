#
# Accelize DRM Reference Design Makefile
#
#  options:
#      None to target On-prem boards
#      AWS=1 to target AWS CSP
#
AWS=0

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

CXXFLAGS += -g -D_DEBUG $(XRT_INC_DIR) -Iinclude
LDFLAGS += $(XRT_LIBDIR)
LDLIBS += $(XRT_LIB) -laccelize_drm -lrt -pthread -lncurses

ifeq ($(AWS),1)
	LDLIBS += -lxrt_aws -lfpga_mgmt
	CSP := -DAWS
	CXXFLAGS += -I$(SDK_DIR)/userspace/include  -Iinclude/awshdk
else
    CXXFLAGS += -I$(SDK_DIR)/userspace/include  -Iinclude/xclhal2
endif

ifeq ($(PROV),1)
	NLPROV := -DNLPROV
endif

all : $(EXENAME) check_env

%.o: %.$(CXX_EXT)
	$(CXX) $(CXXFLAGS) $(MYCFLAGS) $(MYCXXFLAGS) $(CSP) $(NLPROV) -c $< -o $@
	$(CXX) $(CXXFLAGS) $(MYCFLAGS) $(MYCXXFLAGS) -c -MM $< -o $(patsubst %.o, %.d, $@)

$(EXENAME): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) 

clean:
	@rm -rf *.o *.d *.log $(EXENAME)
	
clean_all: clean
	@rm -rf /opt/accelize/drm_demo
	@rm -f /usr/sbin/drmdemo
	@$(eval USERS := $(wildcard /home/*))
	@for userdir in $(USERS); do \
        rm -f $$userdir/Desktop/autorun.sh;\
    done

install:
	@./install_requirements.sh
	@mkdir -p /opt/accelize
	@cp -rf ../drm_demo /opt/accelize/.
	@chmod -R 755 /opt/accelize
	@rm -f /usr/sbin/drmdemo
	@ln -s /opt/accelize/drm_demo/run.sh /usr/sbin/drmdemo
	@$(eval USERS := $(wildcard /home/*))
	@for userdir in $(USERS); do \
        cp autorun.sh $$userdir/Desktop/.;\
    done
    
check_env:
	ifeq ($(AWS),1)
	ifndef SDK_DIR
		@$(error SDK_DIR is undefined. Try "source sdk_setup.sh" to set the software environment)
	endif
	endif

.PHONY: all clean clean_all check_env install

.DEFAULT_GOAL := all
