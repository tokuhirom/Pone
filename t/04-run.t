#!/usr/bin/env perl6 -Ilib
use v6;

use lib 'lib';

use Test;
use Test::Base;
use Pone;

my $dat = slurp('t/basic.dat');

my @blocks = blocks($dat);

plan 1*@blocks;

my $pone = Pone.new;

for @blocks {
    my $title = .input;
    $title ~~ s:g/\n/\\n/;
    subtest {
        my ($out, $sig) = $pone.eval(.input);
        is $out, .expected.chomp, $title;
        ok !$sig.defined, 'no signals';
    }, $title;
}

