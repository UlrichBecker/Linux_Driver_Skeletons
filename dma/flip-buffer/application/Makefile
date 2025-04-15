###############################################################################
##                                                                           ##
##    Makefile for buliding the application-part for driver dmatest-user                    ##
##                                                                           ##
##---------------------------------------------------------------------------##
## File:   ~Linux_Driver_Skeletons/dmaExample2/application/makefile          ##
## Author: Ulrich Becker                                                     ##
## Date:   02.10.2017                                                        ##
###############################################################################
SOURCES =   flip-app.c
EXE_NAME =  fliptest

BASEDIR = .
COMMONDIR = $(BASEDIR)/../../../common/
#SOURCES += $(COMMONDIR)findInstances.c
SOURCES += $(COMMONDIR)terminalHelper.c

VPATH= $(BASEDIR) $(COMMONDIR)
INCDIR = $(BASEDIR) $(COMMONDIR) ./..
CFLAGS = -g -O0

CC     ?=gcc
CFLAGS += $(addprefix -I,$(INCDIR))

OBJDIR=.obj


OBJ = $(addprefix $(OBJDIR)/,$(addsuffix .o,$(notdir $(basename $(SOURCES)))))

.PHONY: all

all: $(EXE_NAME)

$(OBJDIR):
	mkdir $(OBJDIR)

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
