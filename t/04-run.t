#!/usr/bin/env perl6 -Ilib
use v6;

use lib 'lib';

use Test;
use Test::Base;
use Pone;

my $pone = Pone.new;

for blocks($=finish) {
    my $title = .input;
    $title ~~ s:g/\n/\\n/;
    is $pone.eval(.input), .expected.chomp, $title;
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

===
--- input
if False {
    say('not ok');
} elsif True {
    say('ok');
} else {
    say('not ok');
}
--- expected
ok

===
--- input
if False {
    say('not ok');
} elsif False  {
    say('not ok');
} else {
    say('ok');
}
--- expected
ok

===
--- input
say(elems([]))
--- expected
0

===
--- input
say(elems([1,2,3]))
--- expected
3

===
--- input
say 5963
--- expected
5963

===
--- input
my $x = 1111;
say $x;
--- expected
1111

===
--- input
say elems({a => 3});
--- expected
1

===
--- input
say elems({a => 3, b => 4});
--- expected
1

===
--- input
say elems({});
--- expected
0

===
--- input
sub x() { say 3 }
x();
--- expected
3

===
--- input
sub x($n) { say $n }
x(3);
--- expected
3

===
--- input
sub x($n, $m) { say $n-$m }
x(4,9);
--- expected
-5

