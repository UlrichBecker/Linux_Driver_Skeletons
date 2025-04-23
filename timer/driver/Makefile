###############################################################################
##                                                                           ##
##            Makefile for the example of a kernel-timer                     ##
##                                                                           ##
##---------------------------------------------------------------------------##
## File:   Linux_Driver_Skeletons/timer/driver/Makefile                      ##
## Author: Ulrich Becker                                                     ##
## Date:   25.03.1925                                                        ##
###############################################################################
TARGET_NAME := timer
SOURCES := timer-drv.c


CONFIG_TIMER_DRV ?= m
CONFIG_FEATURE_X := y

DEFINES += DEBUG

ccflags-$(CONFIG_FEATURE_X) := -Wno-declaration-after-statement -std=gnu99

ifdef KERNELRELEASE
   # This code-segment becomes invoked from the kernels master-makefile.
   ifdef DEFINES
      #EXTRA_CFLAGS += $(addprefix -D, $(DEFINES))
      ccflags-y += $(addprefix -D, $(DEFINES))
   endif
   obj-$(CONFIG_TIMER_DRV) += $(TARGET_NAME).o
   ifdef SOURCES
      $(TARGET_NAME)-objs := $(patsubst %.c, %.o, $(SOURCES))
   endif
else
   # This code-segment becomes invoked immediately.
   ifdef CROSS_COMPILE
      # Symbolic link or path to the embedded kernel-source-tree of the target-platform.
      KERNEL_SRC_DIR ?= linux
      ARCH ?= arm
   else
      # By default: symbolic link to kernel-source-tree of the host-OS.
      KERNEL_SRC_DIR ?= /lib/modules/$(shell uname -r)/build
   endif

   PWD  := $(shell pwd)

.PHONY: all checkKernelDir clean status rebuild

# Invoke of the kernels master-makefile.
all: checkKernelDir status
ifdef CROSS_COMPILE
	@$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC_DIR) M=$(PWD) modules
else
	@$(MAKE) -C $(KERNEL_SRC_DIR) M=$(PWD) modules
endif

checkKernelDir:
	@if ! [ -d $(KERNEL_SRC_DIR) ]; then \
		echo "ERROR: Can not find the directory or symbolic link \"$(KERNEL_SRC_DIR)\" to the linux-source-tree!" 1>&2; \
		exit 1; \
	fi; \
	if  ! [ -f $(KERNEL_SRC_DIR)/Makefile ]; then \
		echo "ERROR: Source-tree \"$$(readlink -m $(KERNEL_SRC_DIR))\" is invalid!" 1>&2; \
		echo "       Can not find the kernels master-makefile!" 1>&2; \
		exit 1; \
	fi

clean:
	rm -f *.mod.c *.o *.ko modules.order Module.symvers .*.cmd .tmp_versions/* *.livepatch $(TARGET_NAME).mod
	rmdir .tmp_versions

rebuild: clean all

status:
	@printf "Target:   "
ifdef CROSS_COMPILE
	@printf $(ARCH)"\n"
else
	@uname -m
endif
	@printf "Compiler: "
	@$(CROSS_COMPILE)gcc --version | awk -F'\n' '{print $1; exit}'
	@printf "Kernel:   "
	@readlink -m $(KERNEL_SRC_DIR)

ifdef CROSS_COMPILE
#========== Following code sequence is for developing purposes only ===========
$(TARGET_NAME).ko: all

TARGET_DEVICE_USER ?= root
TARGET_DEVICE_IP   ?= 10.0.0.1
TRAGET_DEVICE_DIR  ?= /root

.PHONY: scp
scp: $(TARGET_NAME).ko
	scp $(TARGET_NAME).ko $(TARGET_DEVICE_USER)@$(TARGET_DEVICE_IP):$(TRAGET_DEVICE_DIR)

endif # ifdef CROSS_COMPILE
endif # /ifdef KERNELRELEASE
#=================================== EOF ======================================
