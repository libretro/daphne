#DEBUG=1
ifneq ($(SANITIZER),)
   CFLAGS := -fsanitize=$(SANITIZER) $(CFLAGS)
   CXXFLAGS := -fsanitize=$(SANITIZER) $(CXXFLAGS)
   LDFLAGS  := -fsanitize=$(SANITIZER) $(LDFLAGS)
endif


GIT_VERSION ?= " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
   CXXFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif


ifeq ($(platform),)
	platform = unix
	ifeq ($(shell uname -a),)
		platform = win
	else ifneq ($(findstring MINGW,$(shell uname -a)),)
		platform = win
	else ifneq ($(findstring Darwin,$(shell uname -a)),)
		platform = osx
		arch = intel
	ifeq ($(shell uname -p),powerpc)
		arch = ppc
	endif
	else ifneq ($(findstring win,$(shell uname -a)),)
		platform = win
	endif
	endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
	arch = intel
	ifeq ($(shell uname -p),powerpc)
		arch = ppc
	endif
	else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
endif

TARGET_NAME = daphne

CORE_DIR := .

ifeq ($(platform), unix)
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
   LIBS += -lpthread -ldl
else ifneq (,$(findstring osx,$(platform)))
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   CXXFLAGS += -DOSX
   CFLAGS += -DOSX
	ifeq ($(arch),ppc)
		CXXFLAGS += -D__ppc__ -DOSX_PPC
		CFLAGS += -D__ppc__ -DOSX_PPC
	endif
	OSXVER = `sw_vers -productVersion | cut -d. -f 2`
	OSX_LT_MAVERICKS = `(( $(OSXVER) <= 9)) && echo "YES"`
	fpic += -mmacosx-version-min=10.1
else ifeq ($(platform), pi)
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
   CXXFLAGS +=  -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/vmcs_host/linux
   CFLAGS +=  -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/vmcs_host/linux
   LIBS += -L/opt/vc/lib -lpthread -ldl  -lbcm_host -lvchostif
else ifneq (,$(findstring ios,$(platform)))
   TARGET := $(TARGET_NAME)_libretro_ios.dylib
   fpic := -fPIC
   SHARED := -dynamiclib

ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
endif

   DEFINES := -DIOS  -Wno-error=implicit-function-declaration
   CXXFLAGS += $(DEFINES) -stdlib=libc++
   CFLAGS   += $(DEFINES)
   CC = cc -arch armv7 -isysroot $(IOSSDK)
   CXX = c++ -arch armv7 -isysroot $(IOSSDK)
ifeq ($(platform),ios9)
	CC     += -miphoneos-version-min=8.0
	CXXFLAGS += -miphoneos-version-min=8.0
	CFLAGS += -miphoneos-version-min=8.0
else
	CC     += -miphoneos-version-min=5.0
	CXXFLAGS += -miphoneos-version-min=5.0
	CFLAGS += -miphoneos-version-min=5.0
endif

else ifeq ($(platform), tvos-arm64)
   TARGET := $(TARGET_NAME)_libretro_tvos.dylib
   fpic := -fPIC
   SHARED := -dynamiclib

ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcodebuild -version -sdk appletvos Path)
endif

   DEFINES := -DIOS  -Wno-error=implicit-function-declaration
   CXXFLAGS += $(DEFINES) -stdlib=libc++
   CFLAGS   += $(DEFINES)

else ifneq (,$(findstring qnx,$(platform)))
   TARGET := $(TARGET_NAME)_libretro_qnx.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T

   CC = qcc -Vgcc_ntoarmv7le
   AR = qcc -Vgcc_ntoarmv7le
else ifneq (,$(findstring armv,$(platform)))
   CC ?= gcc
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
   CXXFLAGS += -I.
   CFLAGS += -I.
ifneq (,$(findstring cortexa8,$(platform)))
   CXXFLAGS += -marm -mcpu=cortex-a8
   CFLAGS += -marm -mcpu=cortex-a8
else ifneq (,$(findstring cortexa9,$(platform)))
   CXXFLAGS += -marm -mcpu=cortex-a9
   CFLAGS += -marm -mcpu=cortex-a9
endif
   CXXFLAGS += -marm
   CFLAGS += -marm
ifneq (,$(findstring neon,$(platform)))
   CXXFLAGS += -mfpu=neon
   CFLAGS += -mfpu=neon
endif
ifneq (,$(findstring softfloat,$(platform)))
   CXXFLAGS += -mfloat-abi=softfp
   CFLAGS += -mfloat-abi=softfp
else ifneq (,$(findstring hardfloat,$(platform)))
   CXXFLAGS += -mfloat-abi=hard
   CFLAGS += -mfloat-abi=hard
endif
   CXXFLAGS += -DARM
   CFLAGS += -DARM
   LIBS += -lpthread -ldl

# Classic Platforms ####################
# Platform affix = classic_<ISA>_<µARCH>
# Help at https://modmyclassic.com/comp

# (armv7 a7, hard point, neon based) ### 
# NESC, SNESC, C64 mini 
ifneq (,$(findstring classic_armv7_a7, $(platform)))
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC -pthread
	SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
	CFLAGS += -I. -DARM
	CFLAGS += -Ofast \
	-flto=4 -fwhole-program -fuse-linker-plugin \
	-fdata-sections -ffunction-sections -Wl,--gc-sections \
	-fno-stack-protector -fno-ident -fomit-frame-pointer \
	-falign-functions=1 -falign-jumps=1 -falign-loops=1 \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -fno-unroll-loops \
	-fmerge-all-constants -fno-math-errno \
	-marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
	CXXFLAGS += $(CFLAGS)
	LDFLAGS += -lpthread
	HAVE_NEON = 1
	ARCH = arm
 	LIBS += -lpthread -ldl
	ifeq ($(shell echo `$(CC) -dumpversion` "< 4.9" | bc -l), 1)
	  CFLAGS += -march=armv7-a
	else
	  CFLAGS += -march=armv7ve
	  # If gcc is 5.0 or later
	  ifeq ($(shell echo `$(CC) -dumpversion` ">= 5" | bc -l), 1)
	    LDFLAGS += -static-libgcc -static-libstdc++
	  endif
	endif
endif
# (armv8 a35, hard point, neon based) ###
# Playstation Classic
ifneq (,$(findstring classic_armv8_a35, $(platform)))
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	LIBS += -lpthread -ldl
	SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined -lrt
	CFLAGS += -Ofast -I. -DARM \
	-flto -fwhole-program -fuse-linker-plugin \
	-fdata-sections -ffunction-sections -Wl,--gc-sections \
	-fno-stack-protector -fno-ident -fomit-frame-pointer \
	-falign-functions=1 -falign-jumps=1 -falign-loops=1 \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -fno-unroll-loops \
	-fmerge-all-constants -fno-math-errno \
	-marm -mtune=cortex-a35 -mfpu=neon-fp-armv8 -mfloat-abi=hard
	CXXFLAGS += $(CFLAGS)
	HAVE_NEON = 1
	ARCH = arm
	CFLAGS += -march=armv8-a
	LDFLAGS += -static-libgcc -static-libstdc++
endif
#######################################

# emscripten
else ifeq ($(platform), emscripten)
	TARGET := $(TARGET_NAME)_libretro_emscripten.bc
else
   CC ?= gcc
   TARGET := $(TARGET_NAME)_libretro.dll
   SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=link.T -lwinmm -Wl,--no-undefined
	LIBS += -lwinmm -lws2_32
   CXXFLAGS += -I..
   CFLAGS += -I..
endif

ifeq ($(DEBUG), 1)
   CXXFLAGS += -O0 -g
   CFLAGS += -O0 -g
   LDFLAGS += -g
else
   CXXFLAGS += -O2 -DNDEBUG
   CFLAGS += -O2 -DNDEBUG
endif

ifneq (,$(findstring qnx,$(platform)))
	CFLAGS += -Wc,-std=c99
else
	CFLAGS += -std=c99
endif

include Makefile.common

OBJECTS := $(SOURCES_C:.c=.o) $(SOURCES_CXX:.cpp=.o)
CXXFLAGS += $(fpic)
CFLAGS += -Wall -pedantic $(fpic)

ifneq (,$(findstring osx,$(platform)))
CXXFLAGS += -std=c++11
endif
CFLAGS   += $(INCFLAGS) $(FLAGS)
CXXFLAGS += $(INCFLAGS) $(FLAGS)

ifeq ($(CORE), 1)
   CXXFLAGS += -DCORE
   CFLAGS += -DCORE
endif

CXXFLAGS += -D__LIBRETRO__
CFLAGS += -D__LIBRETRO__

all: $(TARGET)

$(TARGET): $(OBJECTS)

	$(CXX) $(fpic) $(SHARED) $(INCLUDES) -o $@ $(OBJECTS) $(LIBS) $(LDFLAGS) -lm

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(fpic) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) $(fpic) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

