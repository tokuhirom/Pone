use v6;

# use Grammar::Tracer;

# :s => :sigspace. needs spaces between tokens.

grammar Pone::Grammar {
    token TOP { ^ [ <stmts> || '' ] \s* $ }
    token stmts {:s [ <stmt> ||  ';' ]* }

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

    # reserved words
    token keyword {
        [ return | if | unless | while | until | do | class | method | sub | my ] <!ww>
    }

    token stmt:sym<sub> {:s 'sub' <!keyword> <name=ident> '(' <params>? ')' '{' <stmts> '}' }

    token stmt:sym<normal-stmts> {
        <normal-stmt> [ ';'+ <normal-stmt> ]* ';'*
    }
    token stmt:sym<block> {
        <block>
    }

    proto token normal-stmt { * }
    token normal-stmt:sym<term> { <term> }
    token normal-stmt:sym<funcall> { <!keyword> <ident> \s+ <args> }
    # TODO: support `return 1,2,3`
    token normal-stmt:sym<return> {:s 'return' <term> }

    token params {:s
        <term> [ ',' <term> ]*
    }

    token block {
        \s* '{' \s* <stmts> \s* '}' \s*
    }

    rule term { <assign-expr> }

    rule assign-expr {:s <expr(3)> [ '=' <expr(3)> ]? }

    # loosest to tightest
    multi token infix-op(3) { '+' | '-' }
    multi token infix-op(2) { '*' | '/' | '%' }
    multi token infix-op(1) { '**' }

    multi rule expr(0)      { <call> }
    multi rule expr($pred)  {:s <expr($pred-1)> +% [ <infix-op($pred)> ] }

    rule call {
        <value> [ <paren-args>  ]?
    }

    proto rule value { <...> }

    rule value:sym<True> { <sym> }
    rule value:sym<False> { <sym> }
    rule value:sym<funcall> { <!keyword> <ident> '(' <args>? ')' }
    rule paren-args { '(' <args>? ')' }
    rule args { <term> [ ',' <term> ]* }
    rule value:sym<decimal> { <decimal> }
    rule value:sym<string> { <string> }
    rule value:sym<paren> { '(' <term> ')' }
    rule value:sym<array> { '[' <term> [ ',' <term> ]* ','? ']' || '[' ']' }
    rule value:sym<hash> { '{' <hash-pair> [ ',' <hash-pair> ]* ','? '}' || '{' '}' }
    rule value:sym<myvar> {:s 'my' <var> }
    rule value:sym<var> { <var> }
    rule value:sym<closure> { :s 'sub' '(' <params>? ')' '{' <stmts> '}' }
    rule hash-pair { <hash-key> '=>' <term> }
    rule hash-key { <term> || <bare-word> }
    token bare-word { <[A..Z a..z 0..9]>+ }

    token var { \$ <ident> }
    token decimal { '0' || <[+ -]>? <[ 1..9 ]> <[ 0..9 ]>* }
    token ident { <[ A..Z a..z 0..9 ]> <[ A..Z a..z 0..9 ]>* }
    proto rule string { <...> }
    rule string:sym<sqstring> { "'" ( <sqstring-normal> || <sqstring-escape> )+ "'" }
    token sqstring-normal { <-[ \' \\ ]>+ }
    token sqstring-escape { \\ ( \' ) }
}


