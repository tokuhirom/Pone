use v6;

use Grammar::Tracer;

grammar Pone::Grammar {
    token TOP { ^ [ <stmts> || '' ] $ }
    token stmts { <stmt> [ ';' <stmt> ]* }
    token stmt { <funcall> }
    token funcall { <ident> '(' <args> ')' }
    rule args { <term> [ ',' <term> ]* }
    rule term { <value> }

    proto token value { <...> }

    token value:sym<decimal> { <decimal> }
    token value:sym<string> { <string> }

    rule var { \$ <ident> }
    token decimal { '0' || <[ 1..9 ]> <[ 0..9 ]>* }
    token ident { <[ A..Z a..z 0..9 ]> <[ A..Z a..z 0..9 ]>* }
    proto rule string { <...> }
    rule string:sym<sqstring> { "'" ( <sqstring-normal> || <sqstring-escape> )+ "'" }
    token sqstring-normal { <-[ \' \\ ]>+ }
    token sqstring-escape { \\ ( \' ) }
}


