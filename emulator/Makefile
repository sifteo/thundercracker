
BIN = tc-siftulator

OBJS = \
        src/frontend.o \
        src/frontend_cube.o \
        src/main.o \
        src/system.o \
        src/system_network.o \
        src/cube_cpu_core.o \
        src/cube_cpu_disasm.o \
        src/cube_cpu_irq.o \
        src/cube_cpu_opcodes.o \
        src/cube_debug_mainview.o \
        src/cube_debug_memeditor.o \
        src/cube_debug_popups.o \
        src/cube_debug.o \
        src/cube_flash_model.o \
        src/cube_hardware.o \
	src/cube_neighbors.o \

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

CFLAGS += $(shell sdl-config --cflags)
LDFLAGS += $(shell sdl-config --libs)

# OS detection
OS := $(shell uname)
ifneq ($(findstr MINGW32_NT,$(OS)),)  
      OS := windows32
endif

# OpenGL
ifeq ($(OS),Darwin)
        LDFLAGS += -framework OpenGL
else
        LDFLAGS += -lGL
endif

# Curses
ifeq ($(OS),windows32)
        LDFLAGS += ../../deps/win32/pdcurses/pdcurses.lib -lws2_32
else
        LDFLAGS += -lncurses
endif

CFLAGS += -O3 -g -Werror -Wall -Isrc
LDFLAGS += -g -lstdc++
CCFLAGS := $(CFLAGS)

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

%.o: %.c $(CDEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(BIN) $(OBJS)

