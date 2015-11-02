use v6;

use Pone::Tracer;

# :s => :sigspace. needs spaces between tokens.
# Perl5's C<(?=[abc])> becomes C«<?[abc]> - A zero-width positive look-ahead assertion

grammar Pone::Grammar {
    token TOP { ^ [ <stmts> || '' ] \s* $ }
    token stmts {
        <stmt-ish>*
    }

    proto token stmt-ish {*}
    token stmt-ish:sym<normal> {:s
        <normal-stmt> <.terminator> ';'*
    }
    token stmt-ish:sym<stmt> {:s
        <stmt> <.terminator>? ';'*
    }

    token terminator {
        ';'
        || $
        || <?before '}'>
    }

    proto token stmt {*}
    token stmt:sym<if> {:s
        'if' <term> <block> <elsif>* <else>?
    }
    token elsif {:s
        'elsif' <term> <block>
    }
    token else {:s
        'else' <block>
    }

    token stmt:sym<try> {:s
        'try' '{' <stmts> '}'
    }

    token stmt:sym<while> {:s
        'while' <term> '{' <stmts> '}'
    }

    # for $a { ... }
    token stmt:sym<for> {:s
        'for' <term> '{' <stmts> '}'
    }

    # reserved words
    token keyword {
        [ q | qq | for | try | return | if | unless | while | until | do | class | method | sub | my ] <!ww>
    }

    token stmt:sym<sub> {:s 'sub' <!keyword> <name=ident> '(' <params>? ')' '{' <stmts> '}' }

    token stmt:sym<block> {
        <block>
    }

    proto token normal-stmt {*}
    token normal-stmt:sym<funcall> { <!keyword> <ident> \s+ <args> }
    token normal-stmt:sym<term> { <term> }
    # TODO: support `return 1,2,3`
    token normal-stmt:sym<return> {:s 'return' <term> }

    token params {:s
        <term> [ ',' <term> ]*
    }

    token block {
        \s* '{' \s* <stmts> \s* '}' \s*
    }

    rule term { <assign-expr> }

    # Item Assignment Precedence
    rule assign-expr {:s <chaining-binary> [ '=' <chaining-binary> ]? }

    # 16.haining Binary Precedence
    token chaining-binary-op {
        '==' || '!=' || '<=' || '<' || '>=' || '>' ||
        'eq' || 'ne' || 'gt' || 'ge' || 'ge' || 'lt' || 'le' ||
        'before' || 'after' || 'eqv' || '===' || '=:=' || '~~' # TODO
    }
    rule chaining-binary {:s
        <nonchaining-binary> +% [ <chaining-binary-op> ]
    }

    # 15. Nonchaining Binary Precedence(Structural infix)
    rule nonchaining-binary {:s
        <expr(3)> [
            '..' <expr(3)>
        ]?
    }

    # loosest to tightest
    multi token infix-op(3) { '+' | '-' } # 9. Additive Precedence
    multi token infix-op(2) { '*' | '/' | '%' } # 8. Multiplicative Precedence
    multi token infix-op(1) { '**' } # 6. Exponentiation Precedence

    multi rule expr(0)      { <termish> }
    multi rule expr($pred)  {:s <expr($pred-1)> +% [ <infix-op($pred)> ] }

    rule termish {
        <value> [ <postcircumfix> ]*
    }

    proto rule postcircumfix {*}
    rule postcircumfix:sym<call> {
        '(' <args>? ')'
    }
    # TODO $x.$bar()
    rule postcircumfix:sym<method> {
        '.' <ident> '(' <args>? ')'
    }
    rule postcircumfix:sym<at-pos> {
        '[' <term> ']'
    }

    proto rule value {*}

    rule value:sym<True> { <sym> }
    rule value:sym<False> { <sym> }
    rule value:sym<funcall> { <!keyword> <ident> '(' <args>? ')' }
    rule args { <term> [ ',' <term> ]* }
    rule value:sym<number> { <number> }
    rule value:sym<decimal> { <decimal> }
    rule value:sym<string> { <string> }
    rule value:sym<paren> { '(' <term> ')' }
    rule value:sym<array> { '[' <term> [ ',' <term> ]* ','? ']' || '[' ']' }
    rule value:sym<hash> { '{' <hash-pair> [ ',' <hash-pair> ]* ','? '}' || '{' '}' }
    rule value:sym<myvar> {:s 'my' <var> }
    rule value:sym<var> { <var> }
    rule value:sym<err> { '$!' }
    rule value:sym<closure> { :s 'sub' '(' <params>? ')' '{' <stmts> '}' }
    rule value:sym<ident> { <!keyword> <ident> }
    rule hash-pair { <hash-key> '=>' <term> }
    rule hash-key { <term> || <bare-word> }
    token bare-word { <[A..Z a..z 0..9]>+ }

    token var { \$ <ident> }
    token number {
        <[+ -]>? [ '0' || <[ 1..9 ]> <[ 0..9 ]>* ] '.' <[ 0..9 ]>+
    }
    token decimal { '0' || <[+ -]>? <[ 1..9 ]> <[ 0..9 ]>* }
    token ident {
        <!keyword> [
            <[ A..Z a..z _ ]> [
                '-'?  <[ A..Z a..z _ ]>+
                || <[ A..Z a..z 0..9 _ ]>+
            ]*
        ]
    }
    proto rule string {*}
    rule string:sym<sqstring> {
        :my $*STOPPER;
        [
            {$*STOPPER="'";} "'" <sqstring> "'"
            || "q" <sqopen-char> {
                my $open = ~$/<sqopen-char>;
                $*STOPPER = do given ~$/<sqopen-char> {
                    when '(' { ')' }
                    when '[' { ']' }
                    when '{' { '}' }
                    when '<' { '>' }
                    default { $_ }
                }
            } <sqstring> <.stopper>
        ]
    }
    rule sqopen-char {
        <[ ~ @ \[ \( \< ! \{ \, ]>
    }
    token sqstring {
        [
            <!after=stopper> [
                <q=sqstring-escape>
                || <q=sqstring-normal>
            ]
        ]*
    }
    method stopper() {
        self.'!LITERAL'($*STOPPER)
    }
    token sqstring-normal { . }
    token sqstring-escape { \\ ( . ) }
}


