
AUTOMAKE_OPTIONS = foreign subdir-objects
SUBDIRS = man

CFLAGS=-Wall -O2 -I/usr/local/include -Icommon -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\""
LDFLAGS=-L/usr/local/lib -lportaudio -lm -lasound

bin_PROGRAMS = gydra
gydra_SOURCES = src/main.c src/audio.c src/params.c src/devs.c src/net.c common/pa_ringbuffer.c

