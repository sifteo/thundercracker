
BIN = tc-siftulator

OBJS = \
        src/frontend.o \
        src/frontend_cube.o \
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
	src/lodepng.o \
	scripts/data.o \
	scripts/firmware-sbt.o \

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

CDEPS = src/*.h

DATADEPS = \
	scripts/*.glsl \
	scripts/*.png \

# OS detection
OS := $(shell uname)
ifeq ($(findstring MINGW32_NT,$(OS)),MINGW32_NT)  
        OS := windows32
        CC := mingw32-gcc
endif
ifeq ($(OS),windows32)
        DEPS := ../deps/win32
        LDFLAGS += -lws2_32
endif

# SDL (LGPL licensed, so we can't legally statically link to it)
ifeq ($(OS),Darwin)
        CFLAGS += $(shell sdl-config --cflags)
        LDFLAGS += $(shell sdl-config --libs)
else ifeq ($(OS),windows32)
        CFLAGS += -I$(DEPS)/sdl/include 
        LDFLAGS += -lmingw32
        LDFLAGS += $(DEPS)/sdl/lib/libSDL.dll.a
        LDFLAGS += -lwinmm -lgdi32 -lm -mconsole
else
        CFLAGS += $(shell sdl-config --cflags)
        LDFLAGS += $(shell sdl-config --libs)
endif

# OpenGL
ifeq ($(OS),Darwin)
        LDFLAGS += -framework OpenGL
else ifeq ($(OS),windows32)
        OBJS += src/glew/glew.o
        CFLAGS += -I$(DEPS)/glew-1.7.0/include
        LDFLAGS += $(DEPS)/glew-1.7.0/lib/glew32mxs.lib $(DEPS)/glew-1.7.0/lib/glew32s.lib -lOpenGL32 
else
        LDFLAGS += -lGL
endif

# Curses
ifeq ($(OS),windows32)
        # Use our own copy of pdcurses on Win32
        CFLAGS += -Isrc/pdcurses
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
                src/pdcurses/pdcurses/inchstr.o   src/pdcurses/win32/pdcclip.o          \
                src/pdcurses/pdcurses/initscr.o   src/pdcurses/win32/pdcdisp.o          \
                src/pdcurses/pdcurses/inopts.o    src/pdcurses/win32/pdcgetsc.o         \
                src/pdcurses/pdcurses/insch.o     src/pdcurses/win32/pdckbd.o           \
                src/pdcurses/pdcurses/insstr.o    src/pdcurses/win32/pdcscrn.o          \
                src/pdcurses/pdcurses/instr.o     src/pdcurses/win32/pdcsetsc.o         \
                src/pdcurses/pdcurses/kernel.o    src/pdcurses/win32/pdcutil.o          \
                src/pdcurses/pdcurses/keyname.o
else
        # On Linux and Mac OS, curses should already be available
        LDFLAGS += -lncurses
endif

# C++ Libary
ifeq ($(OS),windows32)
        LDFLAGS += -static -lstdc++
else
	LDFLAGS += -lstdc++
endif

# Debug / optimization
#
# Unsurprisingly, compiler optimization produces a very dramatic
# difference in performance. You probably want to leave -O3 on at
# all times unless you have a great reason not to.

CFLAGS += -O3
CFLAGS += -g -Werror -Wall -Isrc
LDFLAGS += -g
CCFLAGS := $(CFLAGS)

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

%.o: %.c $(CDEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

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

scripts/data.cpp: $(DATADEPS)
	python scripts/bin2c.py

firmware:
	python scripts/firmware-sbt.py ../firmware/cube/*.rst

.PHONY: clean firmware

clean:
	rm -f $(BIN) $(OBJS)

