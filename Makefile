CFLAGS = -c -I/usr/include/SDL -g -fPIC
LFLAGS = -lSDL -lSDL_mixer -lSDL_image  -lcurl
COMPILE = gcc -Wall

all: attack

attack: attack.o SFont.o net.o
	$(COMPILE) $(LFLAGS) -o attack attack.o SFont.o net.o

attack.o: attack.c attack.h
	$(COMPILE) $(CFLAGS) -o attack.o attack.c

SFont.o: SFont.c
	$(COMPILE) $(CFLAGS) -o SFont.o SFont.c

net.o: net.c net.h
	$(COMPILE) $(CFLAGS) -o net.o net.c

clean:
	rm attack.o attack
