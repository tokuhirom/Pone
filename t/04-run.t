#!/usr/bin/env perl
use strict;
use warnings;
use lib ' t/extlib/lib/perl5/';
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
        diag $err if $err =~ /\S/;
        if ($err =~ /\d error generated./) {
            fail "$src\n\n$err";
        }
        is($out, $expected, "stdout($src)") or do {
            system("./bin/pone", "-e", $src, "-d");

            if ($ENV{DEBUG}) {
                diag "writing pone_generated.pone: $src";
                open my $fh, '>', 'pone_generated.pone' or die $!; # for deubgging
                print {$fh} $src;
                close $fh;
            }
        };
        ok WIFEXITED($exit), 'exited';
    };
}

