#!/usr/bin/env perl
use strict;
use warnings;
use Test::More;
use Test::Base;
use POSIX;
use Capture::Tiny qw/capture/;

spec_file('t/basic.dat');

plan tests => 1*blocks();

run {
    my $block = shift;
    my ($src, $expected) = ($block->input, $block->expected);

    subtest $src, sub {
        my ($out, $err, $exit) = capture {
            system("./bin/pone", "-e", "$src");
        };
        is $out, $expected, "stdout($src)";
        ok WIFEXITED($exit), 'exited';
    };
}

