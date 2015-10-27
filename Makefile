all: bin/pone.moarvm

test: lib/Pone.pm6.moarvm
	perl t/01-c.t
	perl xt/03-run.t
	perl6-m -Ilib t/02-utils.t
	perl6-m -Ilib t/04-run.t
	perl6-m -Ilib xt/05-run.t

clean:
	rm -f */*.moarvm */*/*.moarvm

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

.PHONY: clean

