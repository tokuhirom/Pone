#!/usr/bin/env perl6 -Ilib
use v6;

use lib 'lib';

use Test;
use Test::Base;
use Pone;

my $pone = Pone.new;

for blocks($=finish) {
    is $pone.eval(.input), .expected.chomp, .input;
}

done-testing;

=finish

===
--- input: say(0)
--- expected
0

===
--- input: say(1)
--- expected
1

===
--- input: say('hello')
--- expected
hello

===
--- input: say(1+2)
--- expected
3

===
--- input: say(1-2)
--- expected
-1

===
--- input: say(1+2+3)
--- expected
6

===
--- input: say(1-2+3)
--- expected
2

===
--- input: say(5*2)
--- expected
10

===
--- input: say(abs(-1))
--- expected
1

===
--- input: say((2+3)*4)
--- expected
20

===
--- input: say(True)
--- expected
True

===
--- input: say(False)
--- expected
False

===
--- input: if True { say(3) }
--- expected
3

===
--- input: if False { say(3) }
--- expected:

===
--- input: say(False)
--- expected
False

===
--- input
if False {
    say('not ok');
} else {
    say('ok');
}
--- expected
ok

===
--- input
if False {
    say('not ok');
} else {
    say('ok');
}
--- expected
ok

