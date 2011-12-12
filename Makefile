WARNINGS=-Wall -Wextra -Wcast-align -Wcast-qual -Wno-cast-qual \
         -Wchar-subscripts -Wcomment -Wdisabled-optimization \
         -Wfloat-equal -Wformat -Wformat=2 -Wformat-nonliteral \
         -Wformat-security -Wformat-y2k -Wimport -Winit-self -Winline \
         -Winvalid-pch -Wlong-long -Wmissing-braces -Wmissing-format-attribute \
         -Wmissing-noreturn -Wpacked -Wparentheses -Wpointer-arith \
         -Wredundant-decls -Wreturn-type -Wsign-compare \
         -Wstrict-aliasing -Wstrict-aliasing=2 -Wswitch -Wswitch-default \
         -Wswitch-enum -Wtrigraphs -Wuninitialized -Wunknown-pragmas \
         -Wunreachable-code -Wunused -Wunused-function -Wunused-label \
         -Wunused-parameter -Wunused-value -Wunused-variable -Wwrite-strings

FLAGS=-g
CFLAGS=$(FLAGS) $(WARNINGS) -Ddebug -Dintro
LDFLAGS=$(FLAGS) -lSDL -lSDL_mixer -lSDL_image -lcurl -lbz2
OBJS=attack.o net.o data.o patchouli.o sfont.o
TARGET=attack

all: $(TARGET)

$(OBJS) : $(wildcard *.h) Makefile .ver

$(TARGET): $(OBJS)

clean:
	rm -f $(OBJS) $(TARGET)

ver: .ver
	@(echo `cat .ver`+0.1 | bc) > newver
	@mv newver .ver
	@echo new version: `cat .ver`

release: ver zip winzip Makefile
	@scp attack root@h.qartis.com:/var/www/qartis.com/www/attack/old/l`cat .ver`
	@scp attack.exe root@h.qartis.com:/var/www/qartis.com/www/attack/old/w`cat .ver`
	@echo released version `cat .ver`
