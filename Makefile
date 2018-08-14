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
CXXFLAGS :=

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

   DEFINES := -DIOS
   CXXFLAGS += $(DEFINES)
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
else ifneq (,$(findstring qnx,$(platform)))
   TARGET := $(TARGET_NAME)_libretro_qnx.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T

   CC = qcc -Vgcc_ntoarmv7le
   AR = qcc -Vgcc_ntoarmv7le
else ifneq (,$(findstring armv,$(platform)))
   CC = gcc
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
# emscripten
else ifeq ($(platform), emscripten)
	TARGET := $(TARGET_NAME)_libretro_emscripten.bc
else
   CC = gcc
   TARGET := $(TARGET_NAME)_libretro.dll
   SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=link.T -lwinmm -Wl,--no-undefined
	LIBS += -lwinmm -lws2_32
   CXXFLAGS += -I..
   CFLAGS += -I..
endif

ifeq ($(DEBUG), 1)
   CXXFLAGS += -O0 -g
   CFLAGS += -O0 -g
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
	$(CXX) $(fpic) $(SHARED) $(INCLUDES) -o $@ $(OBJECTS) $(LIBS) -lm

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(fpic) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) $(fpic) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

