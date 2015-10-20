use v6;

# use Grammar::Tracer;

grammar Pone::Grammar {
    token TOP { :s ^ [ <stmts> || '' ] \s* $ }
    token stmts {:s <stmt> [ ';' <stmt>? ]* }

    proto token stmt { * }
    token stmt:sym<if> {:s
        'if' <term> <block> <elsif>* <else>?
    }
    token elsif {:s
        'elsif' <term> <block>
    }
    token else {:s
        'else' <block>
    }
    token stmt:sym<term> { <term> }
    token stmt:sym<funcall> { :s <ident> <args> }

    rule block {:s
        '{' <stmts> '}'
    }

    rule term { <expr(3)> }

    # loosest to tightest
    multi token infix-op(3) { '+' | '-' }
    multi token infix-op(2) { '*' | '/' | '%' }
    multi token infix-op(1) { '**' }

    multi rule expr(0)      { <value> }
    multi rule expr($pred)  { <expr($pred-1)> +% [ <infix-op($pred)> ] }

    proto rule value { <...> }

    rule value:sym<True> { <sym> }
    rule value:sym<False> { <sym> }
    rule value:sym<funcall> { <ident> '(' <args> ')' }
    rule args { <term> [ ',' <term> ]* }
    rule value:sym<decimal> { <decimal> }
    rule value:sym<string> { <string> }
    rule value:sym<paren> {:s '(' <term> ')' }
    rule value:sym<array> {:s '[' <term> [ ',' <term> ]* ','? ']' || '[' ']' }

    rule var { \$ <ident> }
    token decimal { '0' || <[+ -]>? <[ 1..9 ]> <[ 0..9 ]>* }
    token ident { <[ A..Z a..z 0..9 ]> <[ A..Z a..z 0..9 ]>* }
    proto rule string { <...> }
    rule string:sym<sqstring> { "'" ( <sqstring-normal> || <sqstring-escape> )+ "'" }
    token sqstring-normal { <-[ \' \\ ]>+ }
    token sqstring-escape { \\ ( \' ) }
}


