CFLAGS = -c -I/usr/include/SDL -g -fPIC
LFLAGS = -lSDL -lSDL_mixer -lSDL_image
COMPILE = gcc -Wall

all: attack

attack: attack.o SFont.o
	$(COMPILE) $(LFLAGS) -o attack attack.o SFont.o 

attack.o: attack.c
	$(COMPILE) $(CFLAGS) -o attack.o attack.c

SFont.o: SFont.c
	$(COMPILE) $(CFLAGS) -o SFont.o SFont.c

clean:
	rm attack.o attack
