GBDK_HOME := $(HOME)/gbdk-2020
LCC       := $(GBDK_HOME)/bin/lcc

CFLAGS    := -Wa-l -Wl-m -Wl-j -DUSE_SFR_FOR_REG
LFLAGS    := -Wl-yt0x01

SRCS := src/main.c \
        src/tiles.c \
        src/player.c \
        src/bullet.c \
        src/enemy.c \
        src/boss.c \
        src/collision.c \
        src/hud.c \
        src/sound.c \
        src/pickup.c

OBJS := $(SRCS:.c=.o)

.PHONY: all clean run

all: build/shooter.gb

build/shooter.gb: $(SRCS) | build
	$(LCC) $(CFLAGS) $(LFLAGS) -o $@ $(SRCS)

build:
	mkdir -p build

run: all
	mgba-qt build/shooter.gb

clean:
	rm -f src/*.o src/*.lst src/*.map src/*.sym build/shooter.gb
