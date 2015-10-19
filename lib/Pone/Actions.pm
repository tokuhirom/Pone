unit class Pone::Actions;

use Pone::Utils;

has Set $.builtins = set(<print say dd>);

# TODO: NaN boxing

# Language design:
#
#   String is immutable.

method TOP($/) {
    $/.make: [
        slurp('lib/Pone/runtime.c'),
        "\n",
        "// --------------- ^^^^ rutnime   ^^^^ -------------------",
        "// --------------- vvvv user code vvvv -------------------",
        'int main(int argc, const char **argv) {',
            $/<stmts>.made,
        '}'
    ].join("\n");
}

method stmts($/) {
    $/.make: $/<stmt>».made.map({ "$_;" }).join("\n");
}

method stmt($/) {
    $/.make: $/<funcall>.made;
}

method funcall($/) {
    my $ident = ~$/<ident>;
    if $!builtins{$ident} {
        $/.make: "pone_builtin_" ~ $ident ~ "(" ~ $/<args>.made ~ ")";
    } else {
        $/.make: "pone_user_func_" ~ $ident ~ "(" ~ $/<args>.made ~ ")";
    }
}

method args($/) {
    $/.make: $/<term>».made.join(",");
}

method term($/) {
    $/.make: $/<value>.made;
}

method value:sym<string>($/) {
    $/.make: $/<string>.made;
}

method value:sym<decimal>($/) {
    $/.make: "pone_new_int_mortal(" ~ ~$/ ~ ")";
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
    $m.make: qq!pone_new_str("{escape-c-str($s)}", {$s.encode.bytes})!;
}

method sqstring-normal($/) {
    $/.make: ~$/;
}

method sqstring-escape($/) {
    $/.make: ~$0;
}

