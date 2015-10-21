use v6;
unit class Pone;

use Pone::Actions;
use Pone::Grammar;

has $.cc = 'gcc';

method compile(Str $code, Str :$filename="-") {
    my $actions = Pone::Actions.new(:$filename);
    my $got = Pone::Grammar.parse($code, :$actions);
    if $got {
        return $got.made;
    } else {
        die "parse failed";
    }
}

method !slurp(Str $name) {
    my $src = slurp($name);
    $src ~~ s!^^ \N* '/* PONE_INC */' $$!!;
    qq!#line 1 "$name"\n! ~ $src;
}

method wrap(Str $code) {
    [
        self!slurp('lib/Pone/khash.h'),
        self!slurp('lib/Pone/runtime.c'),
        "\n",
        "// --------------- ^^^^ rutnime   ^^^^ -------------------",
        "// --------------- vvvv user code vvvv -------------------",
        'int main(int argc, const char **argv) {',
        '    pone_world* PONE_WORLD = pone_new_world();',
        '    pone_enter(PONE_WORLD);',
            $code,
        '    pone_leave(PONE_WORLD);',
        '    pone_destroy_world(PONE_WORLD);',
        '}'
    ].join("\n");
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
    open($tmpfile, :w).print(self.wrap($c));
    my $objfile = 'pone_generated.out'; # XXX insecure
    try unlink $objfile;
    run $.cc, '-g', '-std=c99', '-o', $objfile, $tmpfile;
    if so %*ENV<PONE_DEBUG> {
        say "----\n$c\n-----";
    }
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

Pone is a demo software for Perl6' grammar. It has a Perl like syntax.
This compiler compiles code into C.

=head1 FEATURES

=item Reference counting GC

for stability.

=head1 LITERALS

=item C<0>, C<1>, C<100>

There is integer literal.

=item C< 'hoge' >

There is single quotation string literal

=head1 builtin functions

=item C<elems>

Count elements.

=item C<print(Str)>

print message.

=item C<say(Str)>

print message with new line.

=head1 TODO

=item if stmt

=item for stmt

=item while stmt

=item funcall

=item implement p5-ish functions

=head1 AUTHOR

Tokuhiro Matsuno <tokuhirom@gmail.com>

=head1 COPYRIGHT AND LICENSE

Copyright 2015 Tokuhiro Matsuno

This library is free software; you can redistribute it and/or modify it under the Artistic License 2.0.

=end pod
