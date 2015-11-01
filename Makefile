all: blib/libpone.a bin/pone.moarvm

CFLAGS=-std=c99 -g -W -Werror
# CFLAGS+= -DTRACE_REFCNT
# CFLAGS+= -DTRACE_UNIVERSE

OBJFILES= lib/Pone/runtime/obj.o lib/Pone/runtime/class.o lib/Pone/runtime/alloc.o lib/Pone/runtime/array.o lib/Pone/runtime/bool.o lib/Pone/runtime/builtin.o lib/Pone/runtime/code.o lib/Pone/runtime/hash.o lib/Pone/runtime/int.o lib/Pone/runtime/nil.o lib/Pone/runtime/num.o lib/Pone/runtime/op.o lib/Pone/runtime/pone.o lib/Pone/runtime/scope.o lib/Pone/runtime/str.o lib/Pone/runtime/world.o lib/Pone/runtime/universe.o lib/Pone/runtime/iter.o lib/Pone/runtime/exc.o
CTEST_OBJFILES=t/c/assign.o t/c/basic.o t/c/enter.o t/c/func2.o t/c/func.o t/c/hash.o t/c/nop.o t/c/iter.o t/c/for.o t/c/array_methods.o

test: lib/Pone.pm6.moarvm blib/libpone.a $(CTEST_OBJFILES)
	perl t/01-c.t
	perl xt/03-run.t
	perl6-m -Ilib t/02-utils.t
	perl6-m -Ilib t/04-run.t
	perl6-m -Ilib xt/05-run.t

clean:
	rm -f */*.moarvm */*/*.moarvm $(OBJFILES) blib/libpone.a $(CTEST_OBJFILES) vgcore.* core.*

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

lib/Pone/Actions.pm.moarvm: lib/Pone/Actions.pm lib/Pone/Node.pm.moarvm
	perl6-m -Ilib --target=mbc --output=lib/Pone/Actions.pm.moarvm lib/Pone/Actions.pm

lib/Pone/Grammar.pm.moarvm: lib/Pone/Grammar.pm
	perl6-m -Ilib --target=mbc --output=lib/Pone/Grammar.pm.moarvm lib/Pone/Grammar.pm

blib/libpone.a: $(OBJFILES) lib/Pone/runtime/pone.h
	-mkdir -p blib
	ar rcs blib/libpone.a $(OBJFILES)

lib/Pone/runtime/alloc.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/array.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/bool.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/builtin.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/code.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/exc.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/hash.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/int.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/iter.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/nil.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/num.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/op.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/pone.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/scope.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/str.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/universe.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/world.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/class.o: lib/Pone/runtime/pone.h

lib/Pone/runtime/obj.o: lib/Pone/runtime/pone.h

tags:
	rm -f pone_generated.c
	ctags -R .

# yes. hardly copy and pasted. but... i will rewrite all cases after self-hosting.
t/c/assign.o: t/c/assign.c blib/libpone.a
	$(CC) $(CFLAGS) -I lib/Pone/runtime/ -o t/c/assign.o $< blib/libpone.a

t/c/basic.o: t/c/basic.c blib/libpone.a
	$(CC) $(CFLAGS) -I lib/Pone/runtime/ -o t/c/basic.o $< blib/libpone.a

t/c/enter.o: t/c/enter.c blib/libpone.a
	$(CC) $(CFLAGS) -I lib/Pone/runtime/ -o t/c/enter.o $< blib/libpone.a

t/c/func2.o: t/c/func2.c blib/libpone.a
	$(CC) $(CFLAGS) -I lib/Pone/runtime/ -o t/c/func2.o $< blib/libpone.a

t/c/func.o: t/c/func.c blib/libpone.a
	$(CC) $(CFLAGS) -I lib/Pone/runtime/ -o t/c/func.o $< blib/libpone.a

t/c/hash.o: t/c/hash.c blib/libpone.a
	$(CC) $(CFLAGS) -I lib/Pone/runtime/ -o t/c/hash.o $< blib/libpone.a

t/c/nop.o: t/c/nop.c blib/libpone.a
	$(CC) $(CFLAGS) -I lib/Pone/runtime/ -o t/c/nop.o $< blib/libpone.a

t/c/iter.o: t/c/iter.c blib/libpone.a lib/Pone/runtime/pone.h
	$(CC) $(CFLAGS) -I lib/Pone/runtime/ -o t/c/iter.o $< blib/libpone.a

t/c/for.o: t/c/for.c blib/libpone.a lib/Pone/runtime/pone.h
	$(CC) $(CFLAGS) -I lib/Pone/runtime/ -o t/c/for.o $< blib/libpone.a

t/c/array_methods.o: t/c/array_methods.c blib/libpone.a lib/Pone/runtime/pone.h
	$(CC) $(CFLAGS) -I lib/Pone/runtime/ -o t/c/array_methods.o $< blib/libpone.a

pone_generated.out: pone_generated.c blib/libpone.a
	$(CC) $(CFLAGS) -Werror -I lib/Pone/runtime -o ./pone_generated.out  pone_generated.c blib/libpone.a

.PHONY: clean tags

