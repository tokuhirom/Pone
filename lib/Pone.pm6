use v6;
unit class Pone;

use Pone::Actions;
use Pone::Grammar;

my $actions = Pone::Actions.new();

has $.cc = 'gcc';

method compile(Str $code) {
    my $got = Pone::Grammar.parse($code, :$actions);
    if $got {
        return $got.made;
    } else {
        die "parse failed";
    }
}

method eval(Str $code) {
    self!run($code, :capture(True));
}

method run(Str $code) {
    self!run($code, :capture(False));
}

method !run(Str $code, :$capture) {
    my $c = self.compile($code); 

    my $tmpfile = 'pone_generated.c';
    open($tmpfile, :w).print($c);
    my $objfile = 'pone_generated.out'; # XXX insecure
    run $.cc, '-g', '-std=c99', '-o', $objfile, $tmpfile;
    if $capture {
        qqx!./$objfile!;
    } else {
        run "./$objfile";
    }
}

=begin pod

=head1 NAME

Pone - Perl-ish minimal language

=head1 SYNOPSIS

=head1 DESCRIPTION

Pone is a demo software for Perl6' grammar

=head1 AUTHOR

Tokuhiro Matsuno <tokuhirom@gmail.com>

=head1 COPYRIGHT AND LICENSE

Copyright 2015 Tokuhiro Matsuno

This library is free software; you can redistribute it and/or modify it under the Artistic License 2.0.

=end pod
