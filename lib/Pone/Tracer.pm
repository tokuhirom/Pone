use v6;

# MAGICAL_TRASH: This is a trash. But my grammar doesn't work without this.
my class TracedGrammarHOW is Metamodel::GrammarHOW {
    method find_method($obj, $name) {
        my $meth := callsame;
        return $meth if $meth.WHAT.^name eq 'NQPRoutine';
        return $meth unless $meth ~~ Any;
        return $meth unless $meth ~~ Regex;
        return -> $c, |args { $meth($obj, |args); }
    }
}

# Export this as the meta-class for the "grammar" package declarator.
my module EXPORTHOW { }
EXPORTHOW::<grammar> = TracedGrammarHOW;

