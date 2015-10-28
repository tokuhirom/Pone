#!/usr/bin/env perl
use strict;
use warnings;
use Test::More;
use Test::Base;
use Capture::Tiny qw/capture/;

spec_file('t/c-case.dat');

plan tests => 1*blocks();

run {
    my $block = shift;
    my ($src, $expected) = ($block->input, $block->stdout);

    subtest $src, sub {
        (my $objfile = $src) =~ s/\.c$/.o/;

        unlink $objfile;
        system('clang', '-Ilib/Pone/runtime/', '-g', '-std=c99', '-o', $objfile, $src, 'blib/libpone.a');
        my ($out, $err, $exit) = capture {
            system("./$objfile");
        };
        is $out, $expected, "stdout($src)";
    };
}
