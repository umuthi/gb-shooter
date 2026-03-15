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
        src/pickup.c \
        src/dialogue.c

OBJS := $(SRCS:.c=.o)

.PHONY: all clean run

all: build/shooter.gb

build/shooter.gb: $(SRCS) | build
	$(LCC) $(CFLAGS) $(LFLAGS) -o $@ $(SRCS)
	@for f in $(SRCS); do \
	    base=$${f%.c}; \
	    $(LCC) -S $$f -o $${base}.asm 2>/dev/null; \
	    $(GBDK_HOME)/bin/sdasgb -l $${base}.lst $${base}.asm 2>/dev/null; \
	    rm -f $${base}.asm; \
	done

build:
	mkdir -p build

run: all
	mgba-qt build/shooter.gb

clean:
	rm -f src/*.o src/*.asm src/*.lst src/*.map src/*.sym build/shooter.gb
