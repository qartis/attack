FLAGS = -Ddebug -Dintro -O1 -c -g
CFLAGS = $(FLAGS) -I/usr/include -fPIC
LFLAGS = -lSDL -lSDL_mixer -lSDL_image  -lcurl -lbz2
COMPILE = gcc -Wall

WCFLAGS = $(FLAGS) -I/usr/i486-mingw32/include
WLFLAGS = -Lwin -lpthreadGC2 -L/usr/i486-mingw32/lib -lSDL_mixer -lSDL_image -lmingw32 -lSDLmain -lSDL -mwindows /usr/i486-mingw32/lib/libbz2.a /usr/i486-mingw32/lib/libcurl.a /usr/i486-mingw32/lib/libwsock32.a /usr/i486-mingw32/lib/libz.a
WCOMPILE = i486-mingw32-gcc -Wall


all: attack

pack: pack.c
	$(COMPILE) -O2 -o pack pack.c

attack: attack.o SFont.o net.o data.o patch.o pack Makefile
	$(COMPILE) $(LFLAGS) -o attack attack.o SFont.o net.o data.o patch.o
	@./pack attack

attack.o: attack.c attack.h Makefile
	$(COMPILE) $(CFLAGS) -o attack.o attack.c

SFont.o: SFont.c SFont.h
	$(COMPILE) $(CFLAGS) -o SFont.o SFont.c

net.o: net.c attack.h net.h .ver Makefile
	$(COMPILE) $(CFLAGS) -o net.o net.c

data.o: data.c attack.h Makefile
	$(COMPILE) $(CFLAGS) -o data.o data.c

patch.o: patch.c Makefile
	$(COMPILE) $(CFLAGS) -o patch.o patch.c

ver: .ver
	@(echo `cat .ver`+0.1 | bc) > newver
	@mv newver .ver
	@echo new version: `cat .ver`

release: ver zip winzip Makefile
#	@scp attack.zip attack-win.zip .ver root@h.qartis.com:/var/www/qartis.com/www/attack
	@scp attack root@h.qartis.com:/var/www/qartis.com/www/attack/old/l`cat .ver`
	@scp attack.exe root@h.qartis.com:/var/www/qartis.com/www/attack/old/w`cat .ver`
	@echo released version `cat .ver`

zip: attack
	@rm -f attack.zip
	@zip attack.zip attack &> /dev/null
#	@zip attack.zip -r res &> /dev/null
#	@scp attack.zip root@pow:/var/www/qartis.com/labs/attack/

winzip: win
	rm -f attack-win.zip
	zip attack-win.zip attack.exe &> /dev/null
#	@zip attack-win.zip -r res &> /dev/null
	zip attack-win.zip -r -j win/*dll &> /dev/null
#	@scp attack-win.zip root@pow:/var/www/qartis.com/labs/attack

win: attack.exe Makefile

pack.exe: pack.c
	$(WCOMPILE) -O2 -o pack.exe pack.c

attack.exe: wattack.o wSFont.o wnet.o wdata.o wpatch.o pack.exe Makefile
	$(WCOMPILE) wSFont.o wnet.o wattack.o wdata.o wpatch.o -o attack.exe $(WLFLAGS)
	@wine pack.exe attack.exe

wattack.o: attack.c attack.h
	$(WCOMPILE) $(WCFLAGS) -o wattack.o attack.c

wnet.o: net.c net.h attack.h .ver
	$(WCOMPILE) $(WCFLAGS) -o wnet.o net.c

wSFont.o: SFont.c SFont.h
	$(WCOMPILE) $(WCFLAGS) -o wSFont.o SFont.c

wdata.o: data.c
	$(WCOMPILE) $(WCFLAGS) -o wdata.o data.c

wpatch.o: patch.c
	$(WCOMPILE) $(WCFLAGS) -o wpatch.o patch.c
  
clean:
	rm -f *.o attack{,-win}{,.exe,.zip} pack.exe *~

test:
	cd win && SDL_AUDIODRIVER= wine ../attack.exe
