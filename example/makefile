#!/bin/bash

prjPath = .
GLOBAL_LIB ?= lib
BIN ?= bin
GLOBAL_PREFIX ?=

VPATH += .
SOURCES = $(foreach dir,$(VPATH),$(wildcard $(dir)/*.cpp))
HEADERS = $(foreach dir,$(VPATH),$(wildcard $(dir)/*.h))
OBJS = $(SOURCES:%.cpp=%.o)


CC = g++

CFLAGS = -Wall -o2
CFLAGS += -I../include
CFLAGS += -I../include/BasicUsageEnvironment
CFLAGS += -I../include/UsageEnvironment
CFLAGS += -L../lib -lscheduler


TARGET = schedulerdemo




%.d:%cpp
	@set -e; rm -f $@
	$(CC) -MM $(CFLAGS) $^ > $@.$$$;\
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' <$@.$$$ > $@;\
	rm -f $@$$$
	
%.o:%.cpp
	$(CC) $(CFLAGS) -c $^ -o $@
	
$(TARGET):$(OBJS)
	$(CC) $^ $(CFLAGS) -o $@
	cp $@ ../$(BIN)
	rm -rf $@
	
.PHONY : clean
clean:
	rm -f *.o *.bak $(TARGET).* ../$(BIN)/$(TARGET).*