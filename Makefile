
PKGS += "sdl2"

CFLAGS += -Wall $(shell pkg-config --cflags $(PKGS))
LDFLAGS += $(shell pkg-config --libs $(PKGS))

binaural: binaural.o
	$(CC) $^ $(LDFLAGS) -o $@
