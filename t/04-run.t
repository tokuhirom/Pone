#!/usr/bin/env perl6 -Ilib
use v6;

use lib 'lib';

use Test;
use Pone;

my $pone = Pone.new;
is $pone.eval("say(0)"), "0\n";
is $pone.eval("say(1)"), "1\n";
is $pone.eval("say('hello')"), "hello\n";
is $pone.eval("say(1+2)"), "3\n";
is $pone.eval("say(1-2)"), "-1\n";
is $pone.eval("say(1+2+3)"), "6\n";
is $pone.eval("say(1-2+3)"), "2\n";
is $pone.eval("say(5*2)"), "10\n";
is $pone.eval("say(abs(-1))"), "1\n";
is $pone.eval("say(abs(-1))"), "1\n";
is $pone.eval("say((2+3)*4)"), "20\n";
is $pone.eval("say(True)"), "True\n";
is $pone.eval("say(False)"), "False\n";
is $pone.eval('if True { say(3) }'), "3\n";
is $pone.eval('if False { say(3) }'), "";

done-testing;
