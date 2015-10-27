#!/usr/bin/env perl6 -Ilib
use v6;

use lib 'lib';

use Test;
use Test::Base;
use Pone;

my $dat = slurp('t/basic.dat');

my @blocks = blocks($dat);
my $cc = 'clang';

plan 1*@blocks;

my $pone = Pone.new;

for @blocks {
    my $title = .input;
    $title ~~ s:g/\n/\\n/;
    subtest {
        my $c = $pone.compile(.input); 

        my $tmpfile = 'pone_generated.c';
        open($tmpfile, :w).print($c);
        my $objfile = 'pone_generated.out'; # XXX insecure
        try unlink $objfile;
        run $cc, '-g', '-D_POSIX_SOURCE', '-std=c99', '-o', $objfile, $tmpfile;
        if so %*ENV<PONE_DEBUG> {
            say "----\n$c\n-----";
        }

        my $proc = run 'valgrind', "./$objfile", :out, :err;
        my $out = $proc.out.slurp-rest;
        my $err = $proc.err.slurp-rest;
        is $out, .expected.chomp, $title;
        ok !$proc.signal, 'no signals';
        like $err, rx/'All heap blocks were freed'/, 'valgrind';
    }, $title;
}

