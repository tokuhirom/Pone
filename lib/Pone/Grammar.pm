# use Grammar::Tracer;

grammar Pone::Grammar {
    token TOP { ^ [ <stmts> || '' ] $ }
    token stmts { <stmt> [ ';' <stmt> ]* }
    token stmt { <funcall> }
    token funcall { <ident> '(' <args> ')' }
    rule args { <term> [ ',' <term> ]* }
    rule term { <decimal> }

    rule var { \$ <ident> }
    token decimal { '0' || <[ 1..9 ]> <[ 0..9 ]>* }
    token ident { <[ A..Z a..z 0..9 ]> <[ A..Z a..z 0..9 ]>* }

    rule sstr { "'" <sstrchars> "'" }
    token sstrchars {
        \\ \'
    }
    token sstrchar { <-[ \' ]>+ }
}


