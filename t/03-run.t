#!/usr/bin/env perl
use strict;
use warnings;
use Test::More;

unlink 'a.out';
system("gcc -g -DPONE_TESTING -std=c99 lib/Pone/runtime.c");
my $out = qx(./a.out);

is $out, <<'...';
4963
10926
-1314
Hello, world!
True
False
3.140000
3
6
4
3
(undef)
...

done_testing;
