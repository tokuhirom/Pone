#!/usr/bin/env perl
use strict;
use warnings;
use Test::More;
use Test::Base;
use POSIX;
use Capture::Tiny qw/capture/;

plan skip_all => 'valgrind is not available on windows' if $^O eq 'MSWin32';

spec_file('t/basic.dat');

plan tests => 1*blocks();

run {
    my $block = shift;
    my ($src, $expected) = ($block->input, $block->expected);

    subtest $src, sub {
        {
            my ($out, $err, $exit) = capture {
                system("./bin/pone", "-c", "-e", "$src");
            };
            ok WIFEXITED($exit), 'exited';
            is WEXITSTATUS($exit), 0, 'exited by 0';
        }

        {
            my ($out, $err, $exit) = capture {
                system("valgrind", "--leak-check=full", "./pone_generated.out");
            };
            is $out, $expected;
            ok WIFEXITED($exit), 'exited';
            like $err, qr/All heap blocks were freed/, 'valgrind';
        }
    };
}

