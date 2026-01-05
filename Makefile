# MyDesktop Makefile

CC = gcc
CFLAGS = -Wall -O2 `pkg-config --cflags gtk+-3.0 x11`
LDFLAGS = `pkg-config --libs gtk+-3.0 x11`

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
DATADIR = $(PREFIX)/share/mydesktop

SRC = main.c desktop.c panel.c app_menu.c window_manager.c settings.c android.c file_manager.c
OBJ = $(SRC:.c=.o)
TARGET = mydesktop

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(DATADIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/
	install -m 644 theme.css $(DESTDIR)$(DATADIR)/

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all install clean
