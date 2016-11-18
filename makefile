#!/bin/bash

prjPath = .
GLOBAL_LIB ?= lib
LIB =libscheduler.a
GLOBAL_PREFIX ?=

VPATH += ./src/BasicUsageEnvironment
VPATH += ./src/UsageEnvironment
SOURCES = $(foreach dir,$(VPATH),$(wildcard $(dir)/*))
C_SRCS = $(filter %.c,$(SOURCES))
CPP_SRCS = $(filter %.cpp,$(SOURCES))
C_OBJS = $(C_SRCS:%c=%o)
CPP_OBJS = $(CPP_SRCS:%cpp=%o)
OBJS = $(C_OBJS) $(CPP_OBJS)


CC = gcc
GCPP = g++
AR =ar
RANLIB = ranlib


INCLUDE_DIR = -I./include
INCLUDE_DIR += -I./include/BasicUsageEnvironment
INCLUDE_DIR += -I./include/UsageEnvironment

CFLAGS = -Wformat -Wsign-compare  -Wpointer-arith -Wswitch-default -o2
CFLAGS += $(INCLUDE_DIR)



all : $(LIB)

$(LIB) : $(C_OBJS) $(CPP_OBJS)
	$(AR) r $@ $(C_OBJS) $(CPP_OBJS)
	$(RANLIB) $@
	cp $@ $(prjPath)/$(GLOBAL_LIB)
	rm $@
%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDE_DIR) -c $^ -o $@
%.o:%.cpp
	$(GCPP) $(CFLAGS) $(INCLUDE_DIR) -c $^ -o $@
	
clean:
	rm -f $(LIB) $(C_OBJS) $(CPP_OBJS) $../lib/$(LIB)