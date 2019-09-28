CC=gcc
#ncurses is limited to 16 colors; ncursesw has wide char support and 256 colors
LNFLAGS=-lncursesw
CFLAGS=

#Name of the executable to be produced
EXEC = puddle

#For "production" releases, go ahead and optimize
all: CXXFLAGS += -O2
all: $(EXEC)



#add any debug flags (e.g -DDEBUG) here
debug: CFLAGS += -g
debug: $(EXEC)

color: CFLAGS += -DCOLOR
color: $(EXEC)

# Main target
$(EXEC): puddle.c 
	$(CC) puddle.c $(CFLAGS) $(LNFLAGS) -o $(EXEC)



# To remove generated files
clean:
	rm -f $(OBJECTS)

install:
	cp $(EXEC) ~/bin/.
