#!/usr/bin/env perl
use strict;
use warnings;
use Test::More;
use Test::Base;
use Capture::Tiny qw/capture/;

spec_file('t/c-case.dat');

if ($^O eq 'MSWin32') {
  plan skip_all => 'skip failing executable tests on windows';
} else {
  plan tests => 1*blocks();
}

run {
    my $block = shift;
    my ($src, $expected) = ($block->input, $block->stdout);

    subtest $src, sub {
        (my $objfile = $src) =~ s/\.c$/.o/;

        unlink $objfile;
        system('clang', '-D_POSIX_SOURCE', '-Isrc/', '-g', '-std=c99', '-o', $objfile, $src, 'blib/libpone.a');
        my ($out, $err, $exit) = capture {
            system("valgrind --leak-check=full ./$objfile");
        };
        is $out, $expected, "stdout($src)";
        like $err, qr/All heap blocks were freed/, 'valgrind';
    };
}

