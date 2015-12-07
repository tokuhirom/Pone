#!/usr/bin/env perl
use strict;
use warnings;
use lib ' t/extlib/lib/perl5/';
use Test::More;
use Test::Base;
use POSIX;
use Capture::Tiny qw/capture/;
use File::Which;

plan skip_all => 'valgrind is not available on windows' if $^O eq 'MSWin32';
plan skip_all => 'valgrind is not available on darwin' if $^O eq 'darwin';
plan skip_all => "There's no valgrind binary" if !which('valgrind');

spec_file('t/basic.dat');

plan tests => 1*blocks();

run {
    my $block = shift;
    my ($src, $expected) = ($block->input, $block->expected);

    subtest $src, sub {
        my ($out, $err, $exit) = capture {
            system("valgrind", '--suppressions=t/pone.supp', "./bin/pone", '-e', $src);
        };
        is $out, $expected;
        ok WIFEXITED($exit), 'exited';
        is WEXITSTATUS($exit), 0, 'exited by 0';
        like $err, qr/ERROR SUMMARY: 0 errors/, 'valgrind';
    };
}

