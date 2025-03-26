###############################################################################
##                                                                           ##
##    Makefile for buliding the application-part "on-timer"                  ##
##                                                                           ##
##---------------------------------------------------------------------------##
## File:   ~Linux_Driver_Skeletons/timer/application/makefile                ##
## Author: Ulrich Becker                                                     ##
## Date:   16.03.2025                                                        ##
###############################################################################
BASEDIR = .
SOURCES = on-timer.c
EXE_NAME = on-timer

VPATH= $(BASEDIR)
INCDIR = $(BASEDIR)
CFLAGS = -g -O0

CC     ?=gcc
CFLAGS += $(addprefix -I,$(INCDIR))

OBJDIR=.obj


OBJ = $(addprefix $(OBJDIR)/,$(addsuffix .o,$(notdir $(basename $(SOURCES)))))

.PHONY: all 
all: $(EXE_NAME)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.c $(SOURCES) $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXE_NAME): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o $(EXE_NAME) core
	rmdir $(OBJDIR)
	
ifdef CROSS_COMPILE
#========== Following code sequence is for developing purposes only ===========

TARGET_DEVICE_USER ?= root
TARGET_DEVICE_IP   ?= 10.0.0.1
TRAGET_DEVICE_DIR  ?= /root

.PHONY: scp
scp: $(EXE_NAME)
	scp $(EXE_NAME) $(TARGET_DEVICE_USER)@$(TARGET_DEVICE_IP):$(TRAGET_DEVICE_DIR)

endif # ifdef CROSS_COMPILE
#=================================== EOF ======================================
