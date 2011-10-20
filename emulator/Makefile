
BIN = tc-siftulator

INCLUDES += -Isrc
LOCAL_OBJS = \
        src/frontend.o \
        src/frontend_cube.o \
        src/frontend_overlay.o \
	src/lua_script.o \
        src/gl_renderer.o \
        src/main.o \
        src/system.o \
        src/system_network.o \
        src/cube_cpu_core.o \
        src/cube_cpu_disasm.o \
        src/cube_cpu_opcodes.o \
        src/cube_debug_mainview.o \
        src/cube_debug_memeditor.o \
        src/cube_debug_popups.o \
        src/cube_debug.o \
        src/cube_flash_model.o \
        src/cube_hardware.o \
        src/cube_neighbors.o \
	resources/data.o \
	resources/firmware-sbt.o \
	
# For faster 'make' invocation, track dependencies only for our own sources.
OBJS := $(LOCAL_OBJS)
DEPFILES := $(LOCAL_OBJS:.o=.d)

# LodePNG, self-contained PNG encoder/decoder
OBJS += src/lodepng.o

# Box2D physics library
OBJS += \
        src/Box2D/Collision/b2BroadPhase.o \
        src/Box2D/Collision/b2CollideCircle.o \
        src/Box2D/Collision/b2CollideEdge.o \
        src/Box2D/Collision/b2CollidePolygon.o \
        src/Box2D/Collision/b2Collision.o \
        src/Box2D/Collision/b2Distance.o \
        src/Box2D/Collision/b2DynamicTree.o \
        src/Box2D/Collision/b2TimeOfImpact.o \
        src/Box2D/Collision/Shapes/b2ChainShape.o \
        src/Box2D/Collision/Shapes/b2CircleShape.o \
        src/Box2D/Collision/Shapes/b2EdgeShape.o \
        src/Box2D/Collision/Shapes/b2PolygonShape.o \
        src/Box2D/Common/b2BlockAllocator.o \
        src/Box2D/Common/b2Draw.o \
        src/Box2D/Common/b2Math.o \
        src/Box2D/Common/b2Settings.o \
        src/Box2D/Common/b2StackAllocator.o \
        src/Box2D/Common/b2Timer.o \
        src/Box2D/Dynamics/b2Body.o \
        src/Box2D/Dynamics/b2ContactManager.o \
        src/Box2D/Dynamics/b2Fixture.o \
        src/Box2D/Dynamics/b2Island.o \
        src/Box2D/Dynamics/b2World.o \
        src/Box2D/Dynamics/b2WorldCallbacks.o \
        src/Box2D/Dynamics/Contacts/b2ChainAndCircleContact.o \
        src/Box2D/Dynamics/Contacts/b2ChainAndPolygonContact.o \
        src/Box2D/Dynamics/Contacts/b2CircleContact.o \
        src/Box2D/Dynamics/Contacts/b2Contact.o \
        src/Box2D/Dynamics/Contacts/b2ContactSolver.o \
        src/Box2D/Dynamics/Contacts/b2EdgeAndCircleContact.o \
        src/Box2D/Dynamics/Contacts/b2EdgeAndPolygonContact.o \
        src/Box2D/Dynamics/Contacts/b2PolygonAndCircleContact.o \
        src/Box2D/Dynamics/Contacts/b2PolygonContact.o \
        src/Box2D/Dynamics/Joints/b2DistanceJoint.o \
        src/Box2D/Dynamics/Joints/b2FrictionJoint.o \
        src/Box2D/Dynamics/Joints/b2GearJoint.o \
        src/Box2D/Dynamics/Joints/b2Joint.o \
        src/Box2D/Dynamics/Joints/b2MouseJoint.o \
        src/Box2D/Dynamics/Joints/b2PrismaticJoint.o \
        src/Box2D/Dynamics/Joints/b2PulleyJoint.o \
        src/Box2D/Dynamics/Joints/b2RevoluteJoint.o \
        src/Box2D/Dynamics/Joints/b2RopeJoint.o \
        src/Box2D/Dynamics/Joints/b2WeldJoint.o \
        src/Box2D/Dynamics/Joints/b2WheelJoint.o \
        src/Box2D/Rope/b2Rope.o \

# Lua scripting (used for our test suite)
OBJS += \
	src/lua/lapi.o src/lua/lcode.o src/lua/ldebug.o src/lua/ldo.o src/lua/ldump.o \
	src/lua/lfunc.o src/lua/lgc.o src/lua/llex.o src/lua/lmem.o src/lua/lobject.o \
	src/lua/lopcodes.o src/lua/lparser.o src/lua/lstate.o src/lua/lstring.o \
	src/lua/ltable.o src/lua/ltm.o src/lua/lundump.o src/lua/lvm.o src/lua/lzio.o \
	src/lua/lauxlib.o src/lua/lbaselib.o src/lua/ldblib.o src/lua/liolib.o \
	src/lua/lmathlib.o src/lua/loslib.o src/lua/ltablib.o src/lua/lstrlib.o \
	src/lua/loadlib.o src/lua/linit.o src/lua/bit.o \

DATADEPS = \
	resources/*.glsl \
	resources/*.png \

# OS detection
OS := $(shell uname)
ifeq ($(findstring MINGW32_NT,$(OS)),MINGW32_NT)  
        OS := windows32
        CC := mingw32-gcc
endif
ifeq ($(OS),windows32)
        DEPS := ../deps/win32
        LIBS += -lws2_32
endif

# OpenGL
ifeq ($(OS),Darwin)
        LIBS += -framework OpenGL
else ifeq ($(OS),windows32)
        OBJS += src/glew/glew.o
        INCLUDES += -I$(DEPS)/glew-1.7.0/include
        LIBS += $(DEPS)/glew-1.7.0/lib/glew32mxs.lib $(DEPS)/glew-1.7.0/lib/glew32s.lib -lOpenGL32 
else
        LIBS += -lGL
endif

# GLFW, a cross-platform window system layer
INCLUDES += -Isrc/glfw
OBJS += \
    src/glfw/enable.o      src/glfw/window.o src/glfw/joystick.o  src/glfw/thread.o \
    src/glfw/fullscreen.o  src/glfw/init.o   src/glfw/stream.o    src/glfw/time.o   \
    src/glfw/glext.o       src/glfw/input.o
ifeq ($(OS),Darwin)
        LIBS += -framework Cocoa
        INCLUDES += -Isrc/glfw/cocoa
        OBJS += \
            src/glfw/cocoa/cocoa_enable.o      src/glfw/cocoa/cocoa_thread.o    \
            src/glfw/cocoa/cocoa_fullscreen.o  src/glfw/cocoa/cocoa_time.o      \
            src/glfw/cocoa/cocoa_glext.o       src/glfw/cocoa/cocoa_window.o    \
            src/glfw/cocoa/cocoa_init.o        src/glfw/cocoa/cocoa_joystick.o
else ifeq ($(OS),windows32)
        LIBS += -lmingw32 -lwinmm -lgdi32 -lm -mwindows
        INCLUDES += -Isrc/glfw/win32
        OBJS += \
            src/glfw/win32/win32_init.o        src/glfw/win32/win32_joystick.o  \
            src/glfw/win32/win32_enable.o      src/glfw/win32/win32_thread.o    \
            src/glfw/win32/win32_fullscreen.o  src/glfw/win32/win32_time.o      \
            src/glfw/win32/win32_glext.o       src/glfw/win32/win32_window.o
else
        LIBS += -lX11 -lpthread
        INCLUDES += -Isrc/glfw/x11
        OBJS += \
            src/glfw/x11/x11_enable.o      src/glfw/x11/x11_keysym2unicode.o    \
            src/glfw/x11/x11_fullscreen.o  src/glfw/x11/x11_thread.o            \
            src/glfw/x11/x11_glext.o       src/glfw/x11/x11_time.o              \
            src/glfw/x11/x11_init.o        src/glfw/x11/x11_window.o            \
            src/glfw/x11/x11_joystick.o
endif

# Curses
ifeq ($(OS),windows32)
        # Use our own copy of pdcurses on Win32
	# This is the "Win32a" variant, from http://www.projectpluto.com/win32a.htm
        INCLUDES += -Isrc/pdcurses
        OBJS += src/pdcurses/pdcurses/addch.o     src/pdcurses/pdcurses/mouse.o         \
                src/pdcurses/pdcurses/addchstr.o  src/pdcurses/pdcurses/move.o          \
                src/pdcurses/pdcurses/addstr.o    src/pdcurses/pdcurses/outopts.o       \
                src/pdcurses/pdcurses/attr.o      src/pdcurses/pdcurses/overlay.o       \
                src/pdcurses/pdcurses/beep.o      src/pdcurses/pdcurses/pad.o           \
                src/pdcurses/pdcurses/bkgd.o      src/pdcurses/pdcurses/panel.o         \
                src/pdcurses/pdcurses/border.o    src/pdcurses/pdcurses/printw.o        \
                src/pdcurses/pdcurses/clear.o     src/pdcurses/pdcurses/refresh.o       \
                src/pdcurses/pdcurses/color.o     src/pdcurses/pdcurses/scanw.o         \
                src/pdcurses/pdcurses/debug.o     src/pdcurses/pdcurses/scr_dump.o      \
                src/pdcurses/pdcurses/delch.o     src/pdcurses/pdcurses/scroll.o        \
                src/pdcurses/pdcurses/deleteln.o  src/pdcurses/pdcurses/slk.o           \
                src/pdcurses/pdcurses/deprec.o    src/pdcurses/pdcurses/termattr.o      \
                src/pdcurses/pdcurses/getch.o     src/pdcurses/pdcurses/terminfo.o      \
                src/pdcurses/pdcurses/getstr.o    src/pdcurses/pdcurses/touch.o         \
                src/pdcurses/pdcurses/getyx.o     src/pdcurses/pdcurses/util.o          \
                src/pdcurses/pdcurses/inch.o      src/pdcurses/pdcurses/window.o        \
                src/pdcurses/pdcurses/inchstr.o   src/pdcurses/win32a/pdcclip.o          \
                src/pdcurses/pdcurses/initscr.o   src/pdcurses/win32a/pdcdisp.o          \
                src/pdcurses/pdcurses/inopts.o    src/pdcurses/win32a/pdcgetsc.o         \
                src/pdcurses/pdcurses/insch.o     src/pdcurses/win32a/pdckbd.o           \
                src/pdcurses/pdcurses/insstr.o    src/pdcurses/win32a/pdcscrn.o          \
                src/pdcurses/pdcurses/instr.o     src/pdcurses/win32a/pdcsetsc.o         \
                src/pdcurses/pdcurses/kernel.o    src/pdcurses/win32a/pdcutil.o          \
                src/pdcurses/pdcurses/keyname.o
else
        # On Linux and Mac OS, curses should already be available
        LIBS += -lncurses
endif

# Resources
ifeq ($(OS),windows32)
	OBJS += resources/winres.o
endif

# C++ Libary
ifeq ($(OS),windows32)
        LIBS += -static -lstdc++
else
	LIBS += -lstdc++
endif

# Linker flags
ifneq ($(OS),Darwin)
	# Not supported on Apple's LLVM GCC frontend
	LIBS += -Wl,--gc-sections
endif

# Debug / optimization
#
# Unsurprisingly, compiler optimization produces a very dramatic
# difference in performance. You probably want to leave -O3 on at
# all times unless you have a great reason not to.

FLAGS += -O3 -g
WARNFLAGS := -Werror -Wall -Wno-uninitialized

# Automatic dependency generation (*.d files)
FLAGS += -MMD

CFLAGS := $(FLAGS) $(WARNFLAGS) $(INCLUDES)
CCFLAGS := $(FLAGS) $(WARNFLAGS) $(INCLUDES)
MFLAGS := $(FLAGS) $(INCLUDES)
LDFLAGS := $(FLAGS) $(LIBS)

all: $(BIN)

ifeq ($(OS),windows32)
# We like having stripped binaries plus a separate file with
# debug info. Do that on Win32 too.
$(BIN): $(OBJS)
	$(CC) -o src/$(BIN)-d.exe $(OBJS) $(LDFLAGS)
	objcopy --only-keep-debug src/$(BIN)-d.exe $(BIN).dSYM
	strip -s src/$(BIN)-d.exe -o $(BIN).exe
	$(DEPS)/upx307w/upx.exe $(BIN).exe
else
# Default link
$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
endif	
	
%.o: %.cpp
	$(CC) -c $(CCFLAGS) $*.cpp -o $*.o

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o

%.o: %.m
	$(CC) -c $(MFLAGS) $*.m -o $*.o

%.o: %.rc
	windres -i $< -o $@
	
# Data dependencies don't need to be rebuilt by everyone,
# the resulting generated code is checked into git. The
# firmware target is somewhat inconvenient, since it relies
# on the temporary files left over after compiling the cube
# firmware.
#
# Only run the firmware build if you really intend to upgrade
# the simulator to a new stable build of the firmware! Testing
# and debugging of the firmware is much more easily accomplished
# using interpreted mode, and specifying a firmware image on
# the command line.

resources/data.cpp: $(DATADEPS)
	python resources/bin2c.py

firmware:
	python resources/firmware-sbt.py ../firmware/cube/*.rst

.PHONY: clean firmware

clean:
	rm -Rf $(BIN) $(OBJS) $(DEPFILES) $(BIN).exe $(BIN).dSYM

-include $(DEPFILES)
