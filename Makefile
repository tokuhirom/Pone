all: blib/libpone.a bin/pone.moarvm bin/pone

CFLAGS=-std=c99 -g -W -lm -I 3rd/pvip/src/ -DCC=$(CC)
LDFLAGS=-lm
# CFLAGS+= -DTRACE_REFCNT
# CFLAGS+= -DTRACE_UNIVERSE

RUNTIME_OBJFILES= src/obj.o src/class.o src/alloc.o src/array.o src/bool.o src/builtin.o src/code.o src/hash.o src/int.o src/nil.o src/num.o src/op.o src/pone.o src/scope.o src/str.o src/world.o src/universe.o src/iter.o src/exc.o src/range.o
COMPILER_OBJFILES=3rd/pvip/src/pvip_node.o 3rd/pvip/src/pvip_string.o src/compiler/main.o 3rd/pvip/src/gen.pvip.y.o  3rd/pvip/src/gen.node.c
CTEST_OBJFILES=t/c/assign.o t/c/basic.o t/c/enter.o t/c/func2.o t/c/func.o t/c/hash.o t/c/nop.o t/c/iter.o t/c/for.o t/c/array_methods.o

test: lib/Pone.pm6.moarvm blib/libpone.a $(CTEST_OBJFILES)
	prove -lrv t/ xt/

clean:
	rm -f */*.moarvm */*/*.moarvm $(RUNTIME_OBJFILES) blib/libpone.a $(CTEST_OBJFILES) vgcore.* core.* bin/pone

bin/pone.moarvm: lib/Pone/Node.pm.moarvm lib/Pone/Compiler.pm.moarvm lib/Pone/Utils.pm.moarvm lib/Pone.pm6.moarvm
	perl6-m -Ilib --target=mbc --output=bin/pone.moarvm bin/pone

lib/Pone.pm6.moarvm: lib/Pone/Node.pm.moarvm lib/Pone/Compiler.pm.moarvm lib/Pone/Utils.pm.moarvm lib/Pone.pm6 lib/Pone/Actions.pm.moarvm lib/Pone/Grammar.pm.moarvm
	perl6-m -Ilib --target=mbc --output=lib/Pone.pm6.moarvm lib/Pone.pm6

lib/Pone/Compiler.pm.moarvm: lib/Pone/Node.pm.moarvm lib/Pone/Compiler.pm lib/Pone/Utils.pm.moarvm
	perl6-m -Ilib --target=mbc --output=lib/Pone/Compiler.pm.moarvm lib/Pone/Compiler.pm

lib/Pone/Node.pm.moarvm: lib/Pone/Node.pm
	perl6-m -Ilib --target=mbc --output=lib/Pone/Node.pm.moarvm lib/Pone/Node.pm

lib/Pone/Utils.pm.moarvm: lib/Pone/Utils.pm
	perl6-m -Ilib --target=mbc --output=lib/Pone/Utils.pm.moarvm lib/Pone/Utils.pm

lib/Pone/Actions.pm.moarvm: lib/Pone/Actions.pm lib/Pone/Node.pm.moarvm lib/Pone/Utils.pm.moarvm
	perl6-m -Ilib --target=mbc --output=lib/Pone/Actions.pm.moarvm lib/Pone/Actions.pm

lib/Pone/Grammar.pm.moarvm: lib/Pone/Grammar.pm lib/Pone/Tracer.pm.moarvm
	perl6-m -Ilib --target=mbc --output=lib/Pone/Grammar.pm.moarvm lib/Pone/Grammar.pm

lib/Pone/Tracer.pm.moarvm: lib/Pone/Tracer.pm
	perl6-m -Ilib --target=mbc --output=lib/Pone/Tracer.pm.moarvm lib/Pone/Tracer.pm

blib/libpone.a: $(RUNTIME_OBJFILES) src/pone.h
	-mkdir -p blib
	ar rcs blib/libpone.a $(RUNTIME_OBJFILES)

bin/pone: $(COMPILER_OBJFILES)
	$(CC) $(CFLAGS) -o bin/pone $(COMPILER_OBJFILES)

3rd/pvip/src/pvip_node.o: 3rd/pvip/src/pvip_node.c

3rd/pvip/src/pvip_node.c:
	git submodule init
	git submodule update

3rd/pvip/src/gen.node.c:
	cd 3rd/pvip && make src/gen.node.c

3rd/pvip/src/gen.pvip.y.c:
	cd 3rd/pvip && make src/gen.pvip.y.c

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

# yes. hardly copy and pasted. but... i will rewrite all cases after self-hosting.
t/c/assign.o: t/c/assign.c blib/libpone.a
	$(CC) $(CFLAGS) -I src/ -o t/c/assign.o $< blib/libpone.a

t/c/basic.o: t/c/basic.c blib/libpone.a
	$(CC) $(CFLAGS) -I src/ -o t/c/basic.o $< blib/libpone.a

t/c/enter.o: t/c/enter.c blib/libpone.a
	$(CC) $(CFLAGS) -I src/ -o t/c/enter.o $< blib/libpone.a

t/c/func2.o: t/c/func2.c blib/libpone.a
	$(CC) $(CFLAGS) -I src/ -o t/c/func2.o $< blib/libpone.a

t/c/func.o: t/c/func.c blib/libpone.a
	$(CC) $(CFLAGS) -I src/ -o t/c/func.o $< blib/libpone.a

t/c/hash.o: t/c/hash.c blib/libpone.a
	$(CC) $(CFLAGS) -I src/ -o t/c/hash.o $< blib/libpone.a

t/c/nop.o: t/c/nop.c blib/libpone.a
	$(CC) $(CFLAGS) -I src/ -o t/c/nop.o $< blib/libpone.a

t/c/iter.o: t/c/iter.c blib/libpone.a src/pone.h
	$(CC) $(CFLAGS) -I src/ -o t/c/iter.o $< blib/libpone.a

t/c/for.o: t/c/for.c blib/libpone.a src/pone.h
	$(CC) $(CFLAGS) -I src/ -o t/c/for.o $< blib/libpone.a

t/c/array_methods.o: t/c/array_methods.c blib/libpone.a src/pone.h
	$(CC) $(CFLAGS) -I src/ -o t/c/array_methods.o $< blib/libpone.a

pone_generated.out: pone_generated.c blib/libpone.a
	$(CC) $(CFLAGS) -Werror -I src -o ./pone_generated.out  pone_generated.c blib/libpone.a

docs: docs/Array.md docs/Num.md docs/Range.md

docs/Array.md: src/array.c
	pod2markdown src/array.c > docs/Array.md

docs/Num.md: src/num.c
	pod2markdown src/num.c > docs/Num.md

docs/Range.md: src/num.c
	pod2markdown src/range.c > docs/Range.md

.PHONY: clean tags docs

