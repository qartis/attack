CFLAGS = -O1 -c -I/usr/include -g -fPIC
LFLAGS = -lSDL -lSDL_mixer -lSDL_image  -lcurl
COMPILE = gcc -Wall

WCFLAGS = -O1 -c -g -I/usr/local/cross-tools/include
WLFLAGS = -L/usr/local/cross-tools/lib -lSDL_mixer -lSDL_image -lcurl -lmingw32 -lSDLmain -lSDL -mwindows
WCOMPILE = /usr/local/cross-tools/bin/i386-mingw32-gcc -Wall


all: attack

pack: pack.c
	@gcc -Wall -O2 -o pack pack.c
	@strip pack

attack: attack.o SFont.o net.o data.o pack
	$(COMPILE) $(LFLAGS) -o attack attack.o SFont.o net.o data.o
	@./pack attack

attack.o: attack.c attack.h
	$(COMPILE) $(CFLAGS) -o attack.o attack.c

SFont.o: SFont.c SFont.h
	$(COMPILE) $(CFLAGS) -o SFont.o SFont.c

net.o: net.c attack.h net.h .ver
	$(COMPILE) $(CFLAGS) -o net.o net.c

data.o: data.c
	$(COMPILE) $(CFLAGS) -o data.o data.c

ver: .ver
	@(echo `cat .ver`+0.1 | bc) > newver
	@mv newver .ver
	@scp .ver root@pow:/var/www/qartis.com/labs/attack
	@echo new version: `cat .ver`

release: ver zip winzip
	@echo released version `cat .ver`

zip: attack
	@rm -f attack.zip
	@zip attack.zip attack &> /dev/null
#	@zip attack.zip -r res &> /dev/null
	@scp attack.zip root@pow:/var/www/qartis.com/labs/attack/

winzip: win
	@rm -f attack-win.zip
	@zip attack-win.zip attack.exe &> /dev/null
#	@zip attack-win.zip -r res &> /dev/null
	@zip attack-win.zip -r -j /usr/local/cross-tools/runtime/*dll &> /dev/null
	@scp attack-win.zip root@pow:/var/www/qartis.com/labs/attack

win: attack.exe

attack.exe: wattack.o wSFont.o wnet.o wdata.o pack
	$(WCOMPILE) wSFont.o wnet.o wattack.o wdata.o -o attack.exe $(WLFLAGS)
	@./pack attack.exe

wattack.o: attack.c attack.h
	$(WCOMPILE) $(WCFLAGS) -o wattack.o attack.c

wnet.o: net.c net.h attack.h .ver
	$(WCOMPILE) $(WCFLAGS) -o wnet.o net.c

wSFont.o: SFont.c SFont.h
	$(WCOMPILE) $(WCFLAGS) -o wSFont.o SFont.c

wdata.o: data.c
	$(WCOMPILE) $(WCFLAGS) -o wdata.o data.c

clean:
	@rm -f *.o attack{,-win}{,.exe,.zip} *~
