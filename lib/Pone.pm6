use v6;
unit class Pone;

use Pone::Actions;
use Pone::Grammar;
use Pone::Compiler;

has $.cc = 'gcc';

my $aa = '( д) ﾟ ﾟ';

method compile(Str $code, Str :$filename="-") {
    my $compiler = Pone::Compiler.new;
    my $actions = Pone::Actions.new(:$filename);
    my $got = Pone::Grammar.parse($code, :$actions);
    if $got {
        my ($subs, $code) = $compiler.compile($filename, $got.made);
        my $all = slurp('lib/Pone/runtime/pone_all.h');
        my @runtimes;
        for $all.match(/ '"' (\N+?) '"' /, :overlap) -> $m {
            @runtimes.push: self!slurp("lib/Pone/runtime/" ~ ~($m[0]));
        }

        return [
            |@runtimes,
            "\n",
            "// --------------- ^^^^ rutnime       ^^^^ -------------------",
            "// --------------- vvvv user code     vvvv -------------------",
            "// --------------- vvvv functions     vvvv -------------------",
            |$subs,
            "// --------------- vvvv main function vvvv -------------------",
            'int main(int argc, const char **argv) {',
            '    pone_world* world = pone_new_world();',
            '    pone_enter(world);',
                $code,
            '    pone_leave(world);',
            '    pone_destroy_world(world);',
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

=head1 TODO

=item for stmt

=item while stmt

=item user defined funciton

=item implement p5-ish functions

=item closure

=head1 AUTHOR

Tokuhiro Matsuno <tokuhirom@gmail.com>

=head1 COPYRIGHT AND LICENSE

Copyright 2015 Tokuhiro Matsuno

This library is free software; you can redistribute it and/or modify it under the Artistic License 2.0.

=end pod
