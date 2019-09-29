CC=gcc
DESTDIR ?= $(HOME)/bin
#ncurses is limited to 16 colors; ncursesw has wide char support and 256 colors
LNFLAGS=-lncursesw
CFLAGS=

#Name of the executable to be produced
EXEC = puddle

#For "production" releases, go ahead and optimize
all: CFLAGS += -O3
all: $(EXEC)

#add any debug flags (e.g -DDEBUG) here
debug: CFLAGS += -g -DDEBUG
debug: $(EXEC)

nocolor: CFLAGS += -DNOCOLOR -O3
nocolor: $(EXEC)

# Main target
$(EXEC): puddle.c 
	$(CC) puddle.c $(CFLAGS) $(LNFLAGS) -o $(EXEC)

# To remove generated files
clean:
	rm -f $(OBJECTS)

install:
	install -d $(DESTDIR)
	install -m 755 $(EXEC) $(DESTDIR)

uninstall:
	rm $(DESTDIR)/$(EXEC)
