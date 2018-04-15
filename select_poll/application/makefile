###############################################################################
##                                                                           ##
##    Makefile for buliding the application-part "select"                    ##
##                                                                           ##
##---------------------------------------------------------------------------##
## File:   ~Linux_Driver_Skeletons/select_poll/application/makefile          ##
## Author: Ulrich Becker                                                     ##
## Date:   02.10.2017                                                        ##
###############################################################################
BASEDIR = .
SOURCES = select-app.c
EXE_NAME = select

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
	mkdir $(OBJDIR)

$(OBJDIR)/%.o: %.c $(SOURCES) $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXE_NAME): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o $(EXE_NAME) core
	rmdir $(OBJDIR)
	
#=================================== EOF ======================================
