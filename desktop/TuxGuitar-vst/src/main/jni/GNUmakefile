# Makefile for tuxguitar-vst-jni
# This is a fallback for the maven-antrun-plugin build

# Detect platform
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# VST3 SDK and CLAP paths (populated by FetchContent in CMake)
# For Makefile build, we expect them to be available locally or via env vars
VST3_SDK_PATH ?= $(HOME)/vst3sdk
CLAP_PATH ?= $(HOME)/clap

# Java include paths
JAVA_HOME ?= /usr/lib/jvm/default-java
JAVA_INCLUDE = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux

# Compiler flags
CXX = g++
CXXFLAGS = -std=c++17 -fPIC -O2 -Wall -Wextra \
	-I$(VST3_SDK_PATH) \
	-I$(CLAP_PATH)/include \
	$(JAVA_INCLUDE) \
	-DRELEASE

LDFLAGS = -shared -fPIC
LDLIBS = -ldl

# Platform-specific adjustments
ifeq ($(UNAME_S),Darwin)
	LDFLAGS = -dynamiclib -fPIC
	JAVA_INCLUDE = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/darwin
	LDLIBS = -framework CoreFoundation
endif

ifeq ($(UNAME_S),FreeBSD)
	JAVA_INCLUDE = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/freebsd
endif

# Cross-compile for Windows
ifeq ($(CROSS_COMPILE),1)
	CXX = x86_64-w64-mingw32-g++
	LDFLAGS = -shared -fPIC -static -static-libgcc -static-libstdc++
	LDLIBS = -ldl
	JAVA_INCLUDE = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/win32
endif

# Source files
VST3_BASE_CPP = $(wildcard $(VST3_SDK_PATH)/pluginterfaces/base/*.cpp)
VST3_HOSTING_CPP = $(VST3_SDK_PATH)/public.sdk/source/vst/hosting/module.cpp \
	$(VST3_SDK_PATH)/public.sdk/source/vst/hosting/module_linux.cpp \
	$(VST3_SDK_PATH)/public.sdk/source/vst/utility/stringconvert.cpp \
	$(VST3_SDK_PATH)/public.sdk/source/common/commonstringconvert.cpp \
	$(VST3_SDK_PATH)/base/source/fstring.cpp \
	$(VST3_SDK_PATH)/base/source/fobject.cpp \
	$(VST3_SDK_PATH)/base/source/baseiids.cpp \
	$(VST3_SDK_PATH)/base/thread/source/flock.cpp

JNI_SOURCES = VSTWorld.cpp VSTPlugin.cpp VSTComponent.cpp VSTInstance.cpp

# Library name
ifeq ($(UNAME_S),Darwin)
	LIBRARY = libtuxguitar-vst-jni.dylib
else ifeq ($(CROSS_COMPILE),1)
	LIBRARY = tuxguitar-vst-jni.dll
else
	LIBRARY = libtuxguitar-vst-jni.so
endif

all: $(LIBRARY)

$(LIBRARY): $(JNI_SOURCES:.cpp=.o) $(VST3_BASE_CPP:.cpp=.o) $(VST3_HOSTING_CPP:.cpp=.o)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f *.o $(LIBRARY)
	rm -f $(VST3_BASE_CPP:.cpp=.o) $(VST3_HOSTING_CPP:.cpp=.o)

.PHONY: all clean