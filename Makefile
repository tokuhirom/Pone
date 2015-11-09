all: blib/libpone.a bin/pone

CFLAGS=-D_POSIX_C_SOURCE=200809L -std=c99 -g -W -I 3rd/pvip/src/qre/ -I 3rd/linenoise/ -I 3rd/pvip/src/ -DCC=$(CC) -Isrc/ -fPIC -I 3rd/rockre/include/ -ldl -fPIC
LDFLAGS=-lm -lstdc++ -ldl -fPIC
LIBPONE=blib/libpone.a
LIBPVIP=3rd/pvip/libpvip.a
LIBROCKRE=3rd/rockre/librockre.a
# CFLAGS+= -DTRACE_REFCNT
# CFLAGS+= -DTRACE_UNIVERSE

RUNTIME_OBJFILES= src/obj.o src/class.o src/alloc.o src/array.o src/bool.o src/builtin.o src/code.o src/hash.o src/int.o src/nil.o src/num.o src/op.o src/pone.o src/scope.o src/str.o src/world.o src/universe.o src/iter.o src/exc.o src/range.o src/mu.o src/any.o src/cool.o src/socket.o src/re.o
COMPILER_OBJFILES=src/compiler/main.o 3rd/linenoise/linenoise.o

test: blib/libpone.a bin/pone
	prove -lrv t/ xt/

clean:
	rm -f $(RUNTIME_OBJFILES) blib/libpone.a vgcore.* core.* bin/pone
	cd 3rd/pvip/ && make clean

blib/libpone.a: $(RUNTIME_OBJFILES) $(LIBROCKRE) src/pone.h
	-mkdir -p blib
	ar rcs blib/libpone.a $(RUNTIME_OBJFILES) $(LIBROCKRE)

bin/pone: $(COMPILER_OBJFILES) $(LIBPVIP) $(LIBPONE) $(LIBROCKRE)
	$(CC) $(CFLAGS) -o bin/pone $(COMPILER_OBJFILES) $(LIBPONE) $(LIBPVIP) $(LIBROCKRE) $(LDFLAGS)

3rd/pvip/src/pvip_node.o: 3rd/pvip/src/pvip_node.c

3rd/pvip/src/pvip_node.c:
	git submodule init
	git submodule update
	make

$(LIBROCKRE): 3rd/rockre/src/api.cc
	cd 3rd/rockre/ && cmake . && make

3rd/rockre/src/api.cc:
	git submodule init
	git submodule update

3rd/linenoise/linenoise.o: 3rd/linenoise/linenoise.c 3rd/linenoise/linenoise.h

3rd/linenoise/linenoise.c:
	git submodule init
	git submodule update

$(LIBPVIP): 3rd/pvip/src/pvip.h 3rd/pvip/src/pvip_private.h 3rd/pvip/src/pvip.y 3rd/pvip/src/pvip_string.o 3rd/pvip/src/qre/qre.c
	cd 3rd/pvip && make

src/alloc.o: src/pone.h

src/array.o: src/pone.h

src/bool.o: src/pone.h

src/builtin.o: src/pone.h

src/code.o: src/pone.h

src/exc.o: src/pone.h

src/hash.o: src/pone.h

src/int.o: src/pone.h

src/iter.o: src/pone.h

src/nil.o: src/pone.h

src/num.o: src/pone.h

src/op.o: src/pone.h

src/pone.o: src/pone.h

src/scope.o: src/pone.h

src/str.o: src/pone.h

src/universe.o: src/pone.h

src/world.o: src/pone.h

src/class.o: src/pone.h

src/obj.o: src/pone.h

tags:
	rm -f pone_generated.c
	ctags -R .

pone_generated.out: pone_generated.c blib/libpone.a
	$(CC) $(LDFLAGS) $(CFLAGS) -Werror -I src -o ./pone_generated.out  pone_generated.c blib/libpone.a

docs: docs/Array.md docs/Num.md docs/Range.md docs/Mu.md

docs/Array.md: src/array.c
	pod2markdown src/array.c > docs/Array.md

docs/Num.md: src/num.c
	pod2markdown src/num.c > docs/Num.md

docs/Range.md: src/num.c
	pod2markdown src/range.c > docs/Range.md

docs/Mu.md: src/mu.c
	pod2markdown src/mu.c > docs/Mu.md

.PHONY: clean tags docs

