CFLAGS = -g -Wall $(OFLAGS) $(XFLAGS)
OFLAGS = -O3 -DNDEBUG
#OFLAGS = -pg

OBJS = tree.o compile.o

all : greg

greg : greg.o $(OBJS)
	$(CC) $(CFLAGS) -o $@-new greg.o $(OBJS)
	mv $@-new $@

ROOT	=
PREFIX	?= /usr
BINDIR	= $(ROOT)$(PREFIX)/bin

install : $(BINDIR)/greg

$(BINDIR)/% : %
	cp -p $< $@
	strip $@

uninstall : .FORCE
	rm -f $(BINDIR)/greg

greg.o : greg.c

# bootstrap greg from greg.g
greg.c : greg.g compile.c tree.c
	test -f greg && ./greg -o greg-new.c greg.g
	$(CC) $(CFLAGS) -o greg-new greg-new.c $(OBJS)
	./greg-new -o greg-new.c greg.g
	$(CC) $(CFLAGS) -o greg-new greg-new.c $(OBJS)
	mv greg-new.c greg.c
	mv greg-new greg

grammar : .FORCE
	./greg -o greg.c greg.g

clean : .FORCE
	rm -rf *~ *.o *.greg.[cd] greg samples/*.o samples/calc samples/*.dSYM testing1.c testing2.c *.dSYM selftest/

spotless : clean .FORCE
	rm -f greg

samples/calc.c: samples/calc.leg greg
	./greg -o $@ $<

samples/calc: samples/calc.c
	$(CC) $(CFLAGS) -o $@ $<

test: samples/calc run
	echo '21 * 2 + 0' | ./samples/calc | grep 42

run: greg
	mkdir -p selftest
	./greg -o testing1.c greg.g
	$(CC) $(CFLAGS) -o selftest/testing1 testing1.c $(OBJS)
	$(TOOL) ./selftest/testing1 -o testing2.c greg.g
	$(CC) $(CFLAGS) -o selftest/testing2 testing2.c $(OBJS)
	$(TOOL) ./selftest/testing2 -o selftest/calc.c ./samples/calc.leg
	$(CC) $(CFLAGS) -o selftest/calc selftest/calc.c
	$(TOOL) echo '21 * 2 + 0' | ./selftest/calc | grep 42

.FORCE :
