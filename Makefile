#
# Makefile for communication system
#
# $@ 扩展为 当前规则的目标文件名；
# $< 扩展为 当前规则依赖文件列表中的第一个依赖文件；
# $^ 扩展为 整个依赖文件的列表(除掉了所有重复的文件名)。
# $* 扩展为 当前规则中目标文件和依赖文件共享的文件名，不含扩展名；
# $? 扩展为 所有的修改日期比当前规则的目标文件的创建日期更晚的依赖文件，该值只有在使用显式规则时才会被使用；
#
WORKDIR=$(HOME)
#LD_LIBRARY_PATH= $(PWD)/lib
#CXX = g++
LINKER = $(CXX)

CXXFLAGS = -fmessage-length=0
#CXXFLAGS += -Wall
#CXXFLAGS += -Werror
CXXFLAGS += -Wunused-result
#CXXFLAGS += -Wall -D_REENTRANT
#CXXFLAGS += -D_GNU_SOURCE
CXXFLAGS += -g
#CXXFLAGS += -fPIC
#注:-O2 跟踪和调试程序的时候不使用
#CXXFLAGS += -O0
CXXFLAGS += -O3
#CXXFLAGS += -DLINUX
#CXXFLAGS += -DLITTLE_ENDIAN_BYTE_ALIGNED
#CXXFLAGS  += -I ../3th
CXXFLAGS  += -I./core
CXXFLAGS  += -I./app
CXXFLAGS  += -I./kernel

#LDFLAGS += -ld
LDFLAGS += -lst
#LDFLAGS += -lpthread
#LDFLAGS += -L ../3thlib
#LDFLAGS += -L /usr/local/lib

appSRCS = $(shell ls app/*.cpp)
appOBJS = $(subst app/,tmp/app/,$(subst .cpp,.o,$(appSRCS)))

coreSRCS = $(shell ls core/*.cpp)
coreOBJS = $(subst core/,tmp/core/,$(subst .cpp,.o,$(coreSRCS)))

kernelSRCS = $(shell ls kernel/*.cpp)
kernelOBJS = $(subst kernel/,tmp/kernel/,$(subst .cpp,.o,$(kernelSRCS)))

mainSRCS = $(shell ls main/*.cpp)
mainOBJS = $(subst main/,tmp/main/,$(subst .cpp,.o,$(mainSRCS)))

other := samething

SERVER = bin/chatSvr

TARGET := $(SERVER)

all: $(TARGET) 
	@echo 'create: $(TARGET)'
#	@echo 'rm tmp obj ...' #最后会执行此命令,清空临时文件
#	-rm -f $(cOBJS) $(OBJS) $(SERVER_O) $(CLIENT_O)

$(SERVER): $(mainSRCS) $(appSRCS) $(coreOBJS) $(kernelOBJS)
#	@echo 'create OBJ: $(SERVER)'
	$(LINKER) $(CXXFLAGS) $^  $(LDFLAGS) -o $@
#	@echo 'rm tmp obj ...'
#	-rm -f  $(cOBJS) $(OBJS) $(SERVER_O) $(CLIENT_O)

$(CLIENT): $(CLIENT_O) $(cOBJS) $(OBJS)
#	@echo 'create OBJ: $(CLIENT)'
	$(LINKER) $(CXXFLAGS) $^ $(LIBS_CLIENT) $(COMS_SERVER) $(LDFLAGS) -o $@
#	@echo 'rm tmp obj ...'
#	#-rm -f  $(cOBJS) $(OBJS) $(SERVER_O) $(CLIENT_O)

$(appOBJS): tmp/app/%.o: app/%.cpp
	 $(CXX) $(CXXFLAGS) -c $< -o $@

$(coreOBJS): tmp/core/%.o: core/%.cpp
	 $(CXX) $(CXXFLAGS) -c $< -o $@

$(kernelOBJS): tmp/kernel/%.o: kernel/%.cpp
	 $(CXX) $(CXXFLAGS) -c $< -o $@

$(mainOBJS): tmp/main/%.o: main/%.cpp
	 $(CXX) $(CXXFLAGS) -c $< -o $@

tmp/%.o: %.cpp
	 $(CXX) $(CXXFLAGS) -c $< -o $@


.PHONY : clean cleancommon
clean: cleancommon
	-rm -f $(TARGET)

cleancommon:
	-rm -fr tmp/core
	-rm -fr tmp/kernel
	-rm -fr tmp/app
	-rm -fr tmp/main

