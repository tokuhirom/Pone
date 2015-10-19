unit class Pone::Actions;

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
    $/.make: $/<decimal>.made;
}

method decimal($/) {
    $/.make: "pone_new_int_mortal(" ~ ~$/ ~ ")";
}

