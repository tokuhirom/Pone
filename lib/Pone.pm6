use v6;
unit class Pone;

use Pone::Actions;
use Pone::Grammar;
use Pone::Compiler;

has $.cc = 'clang';

my $aa = '( д) ﾟ ﾟ';

method compile(Str $code, Str :$filename="-") {
    my $compiler = Pone::Compiler.new;
    my $actions = Pone::Actions.new(:$filename);
    my $got = Pone::Grammar.parse($code, :$actions);
    if $got {
        my ($subs, $code) = $compiler.compile($filename, $got.made);

        return [
            '#include "pone.h"',
            "\n",
            "// --------------- vvvv functions     vvvv -------------------",
            |$subs,
            "// --------------- vvvv main function vvvv -------------------",
            'int main(int argc, const char **argv) {',
            '    pone_universe* universe;',
            '    pone_world* world;',
            '    pone_init();',
            '    universe = pone_universe_init();',
            '    if (setjmp(universe->err_handlers[0])) {',
            '        pone_universe_default_err_handler(universe);',
            '    }',
            '    world = pone_world_new(universe);',
            "    pone_savetmps(world);",
            "    pone_push_scope(world);",
                $code,
            '    pone_freetmps(world);',
            '    pone_pop_scope(world);',
            '    pone_destroy_world(world);',
            '    pone_universe_destroy(universe);',
            '}'
        ].join("\n");
    } else {
        die "parse failed";
    }
    CATCH { default { die "$aa oops... $_: $code in $filename {.backtrace.full}" } }
}

method !slurp(Str $name) {
    my $src = slurp($name);
    $src ~~ s!^^ \N* '/* PONE_INC */' $$!!;
    qq!#line 1 "$name"\n! ~ $src;
}

sub sig-name($proc) {
    my %sigs = Signal.^enum_values;
    my $sig = %sigs.invert.hash{$proc.signal};
    $sig;
}

method eval(Str $code) {
    my $proc = self!run($code, :out(True));
    return $proc.out.slurp-rest, $proc.signal ?? sig-name($proc) !! Nil;
}

sub dump-ast($node, $level=0) is export {
    my $attr = '';
    if $node.^can('value') {
        $attr = " {$node.value}";
    }

    say((' ' x $level) ~ $node.^name ~ $attr);
    for $node.children {
        if ($_) {
            dump-ast($_, $level+1)
        }
    }
}

method dump-ast(Str $code, :$filename='-e') {
    my $compiler = Pone::Compiler.new;
    my $actions = Pone::Actions.new(:$filename);
    my $got = Pone::Grammar.parse($code, :$actions);
    if $got {
        dump-ast($got.made, 0);
    } else {
        die "cannot parse";
    }
}

method run(Str $code) {
    my $proc = self!run($code, :out($*OUT));
    if $proc.signal {
        $*ERR.say: "$aa SIGNAL received: {sig-name($proc)}";
    }
}

method !run(Str $code, :$out) {
    my $c = self.compile($code); 

    my $tmpfile = 'pone_generated.c';
    open($tmpfile, :w).print($c);
    my $objfile = 'pone_generated.out'; # XXX insecure
    try unlink $objfile;
    run $.cc, '-Isrc/', '-g', '-D_POSIX_SOURCE', '-std=c99', '-o', $objfile, $tmpfile, 'blib/libpone.a';
    if so %*ENV<PONE_DEBUG> {
        say "----\n$c\n-----";
    }

    my $proc = run "./$objfile", :$out;
    return $proc;
}

=begin pod

=head1 NAME

Pone - Perl-ish minimal language

=head1 SYNOPSIS

=head1 DESCRIPTION

Pone is a demo software for Perl6' grammar. It has a Perl6 like syntax.
This compiler compiles Perl6 subset to C.

=head1 FEATURES

=item Reference counting GC

for stability.

=item Optional Precise GC

NYI

=head1 LITERALS

=item C<0>, C<1>, C<100>

There is integer literal.

=item C< 'hoge' >

There is single quotation string literal

=item C< { a => 3 }>

hash literals.

=head1 builtin functions

=item C<elems>

Count elements.

=item C<print(Str)>

print message.

=item C<say(Str)>

print message with new line.

=item C<getenv(Str)>

get environment variable

=item C<time()>

Get current time in sec.

=head1 Bilt-in Types

=head2 Array

There is a literal form.

    [1,2,3]

=head3 methods


=head1 Control statements

=item for stmt

    for [1,2,3] { say $_ }

=item while stmt

    my $i = 1;
    while $i {
        say $i;
        $i = $i - 1;
    }

=head1 TODO

=item class syntax

=item use statement

=item range class

=item range iterator

=item range literal

=item C<%> operator

=item module system

=item precise gc

=item array at-pos operator

    my $ary = [1,2,3];
    say $ary[0];
    $ary[99] = 5;
    say $ary;

=item hash at-pos operator

    my $hash = { a => 3 };
    say $hash<a>;
    $hash<a> = 5;
    say $hash<a>;

=item smart match operator

=item Str#elems method

=item Str#uc method

=item Str#lc method

=item given-when statement

=item file operation support

=item slurp() method

=item pair class

=head1 OPTIMIZATION IDEA

=head2 Compile code to C(done)

This is a basic idea of Pone.

=head2 optimize simple for iteration(todo)

    for 1..10 {
        say $_;
    }

can compile to following code. it means, we can omit the cost to generate iterator.

    for (int i=1; i<=10; ++i) {
        pone_bulitin_say(world, pone_new_int(i));
    }

if it's realy small code, we can use 'loop unrolling' technique.
(but normally, C compiler can do it)

=head1 MILESTONE

=head2 Hello, world

    say 'Hello, world'

Done.

=head2 Basic FizzBuzz support

Following FizzBuzz forms should work.

    for 1..100 -> $n {
        given $n {
            when $_ % 15 == 0 { say "FizzBuzz" }
            when $_ %  3 == 0 { say "Fizz"     }
            when $_ %  5 == 0 { say "Buzz"     }
            default           { say $n         }
        }
    }

    for 1..100 {
        given $_ {
            when $_ % 15 == 0 { say "FizzBuzz" }
            when $_ %  3 == 0 { say "Fizz"     }
            when $_ %  5 == 0 { say "Buzz"     }
            default           { say $n         }
        }
    }

=head2 Basic Regular Expression support

Should we compile regular expression to C code?

=head2 Basic grammar support

=head2 Self hosting

Is a dream...

=head1 AUTHOR

Tokuhiro Matsuno <tokuhirom@gmail.com>

=head1 COPYRIGHT AND LICENSE

Copyright 2015 Tokuhiro Matsuno

This library is free software; you can redistribute it and/or modify it under the Artistic License 2.0.

=end pod
