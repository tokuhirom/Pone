use v6;

use Pone::Node;

unit class Pone::Actions;

use Pone::Utils;

has $.filename = "-";

# Language design:
#
#   String is immutable.

# See HLL::Compiler
sub lineof($/) {
    my $n=1;
    $/.orig.substr(0, $/.from).subst(/\n/, {$n++});
    $n;
}

method TOP($/) {
    $/.make: $/<stmts>.made;
}

method stmts($/) {
    $/.make: Pone::Node::Stmts.new(
        $/<stmt-ish>.map({
            my $node = $_.ast;
            $node // die "undefined node!";
            $node.lineno = lineof($_);
            $node
        })
    );
}

method stmt-ish:sym<normal>($/) {
    $/.make: $/<normal-stmt>.made;
}

method stmt-ish:sym<stmt>($/) {
    $/.make: $/<stmt>.made;
}

method stmt:sym<if>($/) {
    my $if = Pone::Node::If.new([
        $/<term>.ast, $/<block>.made,
    ]);
    my $cur = $if;
    if $/<elsif>:exists {
        for $/<elsif> {
            my $elsif = .ast;
            $cur.children.push: $elsif;
            $cur = $elsif;
        }
    }
    if $/<else>:exists {
        $cur.children.push: $/<else>.ast;
    }

    $/.make: $if;
}

method stmt:sym<try>($/) {
    my $try = Pone::Node::Try.new($/<stmts>.made);
    $try.lineno = lineof($/);
    $/.make: $try;
}

method stmt:sym<for>($/) {
    my $for = Pone::Node::For.new([$/<term>.made, $/<stmts>.made]);
    $for.lineno = lineof($/);
    $/.make: $for;
}

method stmt:sym<while>($/) {
    my $stmt = Pone::Node::While.new([$/<term>.made, $/<stmts>.made]);
    $stmt.lineno = lineof($/);
    $/.make: $stmt;
}

method stmt:sym<block>($/) {
    $/.make: $/<block>.made;
}

method elsif($/) {
    $/.make: Pone::Node::If.new(
        [ $/<term>.ast, $/<block>.ast ],
    );
}

method else($/) {
    $/.make: Pone::Node::If.new(
        [Pone::Node::True.new(), $/<block>.made]
    );
}

method block($/) {
    $/.make: Pone::Node::Block.new($/<stmts>.ast);
}

method term($/) { $/.make: $/<assign-expr>.ast }
method assign-expr($/) {
    my $l = $/<chaining-binary>[0].made;
    if $/<chaining-binary>.elems == 2 {
        $/.make: Pone::Node::Assign.new([$l, $/<chaining-binary>[1].made]);
    } else {
        $/.make: $l;
    }
}

method chaining-binary($/) {
    my @e = $/<nonchaining-binary>».made;
    my @ops = $/<chaining-binary-op>».Str;
    my $l = @e.shift;
    while @e.elems {
        my $op = @ops.shift;
        my $r = @e.shift;
        given $op {
            when '==' {
                $l = Pone::Node::EQ.new([$l, $r]);
            }
            when '!=' {
                $l = Pone::Node::NE.new([$l, $r]);
            }
            when '<=' {
                $l = Pone::Node::LE.new([$l, $r]);
            }
            when '<' {
                $l = Pone::Node::LT.new([$l, $r]);
            }
            when '>=' {
                $l = Pone::Node::GE.new([$l, $r]);
            }
            when '>' {
                $l = Pone::Node::GT.new([$l, $r]);
            }
            # TODO string comparision operators
            default {
                die "unknown operatar: '$op'";
            }
        }
    }
    $/.make: $l;
}

method nonchaining-binary($/) {
    my $l = $/<expr>[0].made;
    if $/<expr>.elems == 2 {
        $/.make: Pone::Node::Range.new([$l, $/<expr>[1].made]);
    } else {
        $/.make: $l;
    }
}

# see integration/99problems-41-to-50.t in roast
method expr($/) {
    if $/<termish> {
        $/.make: $/<termish>.made;
    } else {
        my @e = $/<expr>».made;
        my @ops = $/<infix-op>».made;
        my $l = @e.shift;
        while @e.elems {
            my $op = @ops.shift;
            my $r = @e.shift;
            given $op {
                when '+' { $l = Pone::Node::Add.new([$l, $r]); }
                when '-' { $l = Pone::Node::Subtract.new([$l, $r]); }
                when '*' { $l = Pone::Node::Multiply.new([$l, $r]); }
                when '/' { $l = Pone::Node::Divide.new([$l, $r]); }
                when '%' { $l = Pone::Node::Mod.new([$l, $r]); }
                default { die "unknown operatar: $op"; }
            }
        }
        $/.make: $l;
    }
}

method infix-op($/) { $/.make: ~$/ }

method !funcall($/) {
    my @children = Pone::Node::Ident.new(~$/<ident>);
    if $/<args> {
        @children.push: |$/<args>.made;
    }
    $/.make: Pone::Node::Funcall.new(@children);
}

method normal-stmt:sym<term>($/) {
    $/.make: $/<term>.made;
}

method normal-stmt:sym<return>($/) {
    $/.make: Pone::Node::Return.new([$/<term>.made]);
}

method normal-stmt:sym<funcall>($/) {
    self!funcall($/);
}

method stmt:sym<sub>($/) {
    my $node = Pone::Node::Sub.new(
        [$/<name>.ast, $/<params>.ast, $/<stmts>.ast]
    );
    $node.lineno = lineof($/);
    $/.make: $node;
}

method params($/) {
    $/.make: Pone::Node::Params.new($/<term>».ast);
}

method ident($/) {
    $/.make: Pone::Node::Ident.new(~$/);
}

method value:sym<funcall>($/) {
    self!funcall($/);
}

method value:sym<err>($/) {
    $/.make: Pone::Node::ErrVar.new();
}

method termish($/) {
    my $v = $/<value>.made;
    for $/<postcircumfix>».made {
        .children.unshift($v);
        $v = $_;
    }
    $/.make: $v;
}

method postcircumfix:sym<call>($/) {
    $/.make: Pone::Node::Funcall.new(
        [$/<args>.made || Pone::Node::Args.new()]
    );
}

method postcircumfix:sym<at-pos>($/) {
    $/.make: Pone::Node::AtPos.new([$/<term>.made]);
}

method postcircumfix:sym<method>($/) {
    $/.make: Pone::Node::MethodCall.new([
        $/<ident>.made,
        $/<args>.made || Pone::Node::Args.new()
    ]);
}

method args($/) {
    $/.make: Pone::Node::Args.new($/<term>».made);
}

method value:sym<string>($/) {
    $/.make: $/<string>.made;
}

method value:sym<number>($/) {
    $/.make: Pone::Node::Num.new($/.Str.Num);
}

method value:sym<decimal>($/) {
    $/.make: Pone::Node::Int.new($/.Str.Int);
}

method value:sym<paren>($/) {
    $/.make: $/<term>.ast;
}

method value:sym<True>($/) {
    $/.make: Pone::Node::True.new();
}

method value:sym<False>($/) {
    $/.make: Pone::Node::False.new();
}

method value:sym<array>($/) {
    $/.make: Pone::Node::Array.new($/<term>».ast);
}

method value:sym<hash>($/) {
    $/.make: Pone::Node::Hash.new(
        $/<hash-pair>».made
    );
}

method value:sym<myvar>($/) {
    $/.make: Pone::Node::My.new($/<var>.ast);
}

method value:sym<var>($/) {
    $/.make: $/<var>.ast;
}

method value:sym<closure>($/) {
    my $node = Pone::Node::Sub.new(
        [Pone::Node::Nil.new, $/<params>.ast, $/<stmts>.ast]
    );
    $node.lineno = lineof($/);
    $/.make: $node;
}

method value:sym<ident>($/) {
    $/.make: $/<ident>.made;
}

method hash-pair($/) {
    $/.make: Pone::Node::Pair.new(
        [$/<hash-key>.made, $/<term>.made]
    );
}

method hash-key($/) {
    $/.make: $/<term> ?? $/<term>.ast !! Pone::Node::Str.new(~$/<bare-word>);
}

method var($/) {
    $/.make: Pone::Node::Var.new(~$/);
}

method string:sym<sqstring>($/) {
    $/.make: $/<sqstring>.made;
}

method sqstring($/) {
    $/.make: Pone::Node::Str.new($/<q>».made.join(""));
}

method sqstring-normal($/) {
    $/.make: ~$/;
}

method sqstring-escape($/) {
    $/.make: ~$0;
}


