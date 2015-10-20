use v6;

# use Grammar::Tracer;

grammar Pone::Grammar {
    token TOP { ^ [ <stmts> || '' ] $ }
    token stmts { <stmt> [ ';' <stmt> ]* }
    token stmt { <term> }

    rule term { <expr(3)> }

    # loosest to tightest
    multi token infix-op(3) { '+' | '-' }
    multi token infix-op(2) { '*' | '/' | '%' }
    multi token infix-op(1) { '**' }

    multi rule expr(0)      { <value> }
    multi rule expr($pred)  { <expr($pred-1)> +% [ <infix-op($pred)> ] }

    proto token value { <...> }

    token value:sym<funcall> { <ident> '(' <args> ')' }
    rule args { <term> [ ',' <term> ]* }
    token value:sym<decimal> { <decimal> }
    token value:sym<string> { <string> }
    token value:sym<paren> { '(' <term> ')' }
    token value:sym<true> { 'True' }
    token value:sym<false> { 'False' }

    rule var { \$ <ident> }
    token decimal { '0' || <[+ -]>? <[ 1..9 ]> <[ 0..9 ]>* }
    token ident { <[ A..Z a..z 0..9 ]> <[ A..Z a..z 0..9 ]>* }
    proto rule string { <...> }
    rule string:sym<sqstring> { "'" ( <sqstring-normal> || <sqstring-escape> )+ "'" }
    token sqstring-normal { <-[ \' \\ ]>+ }
    token sqstring-escape { \\ ( \' ) }
}


