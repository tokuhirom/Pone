#!/usr/bin/env perl
use strict;
use warnings;
use Test::More;
use Test::Base;
use Capture::Tiny qw/capture/;

plan tests => 1*blocks();

run {
    my $block = shift;
    my ($src, $expected) = ($block->input, $block->stdout);

    subtest $src, sub {
        (my $objfile = $src) =~ s/\.c$/.o/;

        unlink $objfile;
        system('gcc', '-Ilib/Pone/runtime/', '-g', '-std=c99', '-o', $objfile, $src);
        my ($out, $err, $exit) = capture {
            system("valgrind ./$objfile");
        };
        is $out, $expected, "stdout($src)";
        like $err, qr/All heap blocks were freed/, 'valgrind';
    };
}

__END__

===
--- input: t/c/hash.c
--- stdout
2

===
--- input: t/c/enter.c
--- stdout:

===
--- input: t/c/basic.c
--- stdout
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

===
--- input: t/c/assign.c
--- stdout
4963
5555

===
--- input: t/c/func.c
--- stdout
102

===
--- input: t/c/func2.c
--- stdout
1

===
--- input: t/c/nop.c
--- stdout

