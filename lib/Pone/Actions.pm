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
        $/<stmt>.map({
            my $node = $_.ast;
            $node.lineno = lineof($_);
            $node
        })
    );
}

method stmt:sym<normal-stmts>($/) {
    $/.make: Pone::Node::Stmts.new(
        $/<normal-stmt>.map({
            my $node = $_.ast;
            $node.lineno = lineof($_);
            $node
        })
    );
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
    my $l = $/<expr>[0].made;
    if $/<expr>.elems == 2 {
        $/.make: Pone::Node::Assign.new([$l, $/<expr>[1].made]);
    } else {
        $/.make: $l;
    }
}

# see integration/99problems-41-to-50.t in roast
method expr($/) {
    if $/<call> {
        $/.make: $/<call>.made;
    } else {
        my @e = $/<expr>».made;
        my @ops = $/<infix-op>».made;
        my $l = @e.shift;
        while @e.elems {
            my $op = @ops.shift;
            my $r = @e.shift;
            given $op {
                when '+' {
                    $l = Pone::Node::Add.new([$l, $r]);
                }
                when '-' {
                    $l = Pone::Node::Subtract.new([$l, $r]);
                    # $l = "pone_subtract(PONE_WORLD, $l,$r)";
                }
                when '*' {
                    $l = Pone::Node::Multiply.new([$l, $r]);
                    # $l = "pone_multiply(PONE_WORLD, $l,$r)";
                }
                when '/' {
                    $l = Pone::Node::Divide.new([$l, $r]);
                    # $l = "pone_divide(PONE_WORLD, $l,$r)";
                }
                when '%' {
                    $l = Pone::Node::Mod.new([$l, $r]);
                    # $l = "pone_mod(PONE_WORLD, $l,$r)";
                }
                default {
                    die "unknown operatar: $op";
                }
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

method call($/) {
    if $/<paren-args> {
        $/.make: Pone::Node::Funcall.new(
            [$/<value>.made, $/<paren-args>.made]
        );
    } else {
        $/.make: $/<value>.made;
    }
}

method value:sym<funcall>($/) {
    self!funcall($/);
}

method paren-args($/) {
    $/.make: $/<args>.made || Pone::Node::Args.new();
}

method args($/) {
    $/.make: Pone::Node::Args.new($/<term>».made);
}

method value:sym<string>($/) {
    $/.make: $/<string>.made;
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

# XXX bad code
method string:sym<sqstring>($m) {
    my @s;
    for $m[0].list -> $val {
        my $v = $val.Str;
        if $val.Str ~~ /^\\(.)$/ {
            @s.push: ~$0;
        } else {
            @s.push: ~$v;
        }
    }
    my $s = @s.join("");
    $m.make: Pone::Node::Str.new($s);
}

method sqstring-normal($/) {
    $/.make: ~$/;
}

method sqstring-escape($/) {
    $/.make: ~$0;
}


