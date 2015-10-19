#!/usr/bin/env perl6 -Ilib
use v6;

use lib 'lib';

use Test;
use Pone;

my $pone = Pone.new;
is $pone.eval("say(0)"), "0\n";
is $pone.eval("say(1)"), "1\n";
is $pone.eval("say('hello')"), "hello\n";

done-testing;

