unit class Pone::Actions;

use Pone::Utils;

has Set $.builtins = set(<print say dd abs elems>);
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
    $/.make: $/<stmt>.map({
        my $line = lineof($_);
        qq!#line $line "$.filename"\n! ~ .made ~ ";\n"
    }).join("\n");
}

method stmt:sym<term>($/) {
    $/.make: $/<term>.made;
}

method stmt:sym<if>($/) {
    my $c = "if (pone_so({$/<term>.made}))\n{ $/<block>.made }";
    if $/<elsif>:exists {
        $c ~= $/<elsif>».made.join("\n");
    }
    if $/<else>:exists {
        $c ~= $/<else>.made;
    }
    $/.make: $c;
}

method elsif($/) {
    $/.make: ' else if (pone_so(' ~ $/<term>.made ~ "))\n" ~ $/<block>.made;
}

method else($/) {
    $/.make: "else\n" ~ $/<block>.made;
}

method block($/) {
    $/.make: '{ pone_enter(PONE_WORLD);' ~ "\n" ~ $/<stmts>.made ~ "\n" ~ ' pone_leave(PONE_WORLD); }';
}

method term($/) { $/.make: $/<expr>.made }

# see integration/99problems-41-to-50.t in roast
method expr($/) {
    if $/<value> {
        $/.make: $/<value>.made;
    } else {
        my @e = $/<expr>».made;
        my @ops = $/<infix-op>».made;
        my $l = @e.shift;
        while @e.elems {
            my $op = @ops.shift;
            my $r = @e.shift;
            given $op {
                when '+' {
                    $l = "pone_add(PONE_WORLD, $l,$r)";
                }
                when '-' {
                    $l = "pone_subtract(PONE_WORLD, $l,$r)";
                }
                when '*' {
                    $l = "pone_multiply(PONE_WORLD, $l,$r)";
                }
                when '/' {
                    $l = "pone_divide(PONE_WORLD, $l,$r)";
                }
                when '%' {
                    $l = "pone_mod(PONE_WORLD, $l,$r)";
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
    my $ident = ~$/<ident>;
    if $!builtins{$ident} {
        $/.make: "pone_builtin_" ~ $ident ~ "(PONE_WORLD, " ~ $/<args>.made ~ ")";
    } else {
        $/.make: "pone_user_func_" ~ $ident ~ "(PONE_WORLD, " ~ $/<args>.made ~ ")";
    }
}

method stmt:sym<funcall>($/) {
    self!funcall($/);
}

method value:sym<funcall>($/) {
    self!funcall($/);
}

method args($/) {
    $/.make: $/<term>».made.join(",");
}

method value:sym<string>($/) {
    $/.make: $/<string>.made;
}

method value:sym<decimal>($/) {
    $/.make: "pone_mortalize(PONE_WORLD, pone_new_int(PONE_WORLD, " ~ ~$/ ~ "))";
}

method value:sym<paren>($/) {
    $/.make: "(" ~ $/<term>.made ~ ")";
}

method value:sym<True>($/) {
    $/.make: "pone_true()";
}

method value:sym<False>($/) {
    $/.make: "pone_false()";
}

method value:sym<array>($/) {
    if $/<term> {
        $/.make: 'pone_mortalize(PONE_WORLD, pone_new_ary(PONE_WORLD, ' ~ $/<term>.elems ~ "," ~ $/<term>».made.join(",") ~ '))';
    } else {
        $/.make: 'pone_mortalize(PONE_WORLD, pone_new_ary(PONE_WORLD, 0))';
    }
}

method value:sym<hash>($/) {
    my $c = 'pone_mortalize(PONE_WORLD, pone_new_hash(PONE_WORLD, ';
    if $/<hash-pair> {
        $c ~= $/<hash-pair>.elems ~ "," ~ $/<hash-pair>».made.join(",");
    } else {
        $c ~= "0";
    }
    $c ~= '))';
    $/.make: $c;
}

method hash-pair($/) {
    $/.make: $/<hash-key>.made => $/<term>.made;
}

method hash-key($/) {
    $/.make: $/<term> ?? $/<term>.made !! ~$/<bare-word>;
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
    $m.make: qq!pone_new_str(PONE_WORLD, "{escape-c-str($s)}", {$s.encode.bytes})!;
}

method sqstring-normal($/) {
    $/.make: ~$/;
}

method sqstring-escape($/) {
    $/.make: ~$0;
}


