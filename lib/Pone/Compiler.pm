use v6;

use Pone::Node;

unit class Pone::Compiler;

use Pone::Utils;

has $!filename;
has Set $.builtins = set(<print say dd abs elems>);

sub mortal(Str $s) {
    "pone_mortalize(PONE_WORLD, $s)"
}

method !infix(Str $func, Pone::Node $node) {
    sprintf('%s(PONE_WORLD, %s, %s)',
        $func,
        self!compile($node.children[0]),
        self!compile($node.children[1]));
}

method compile(Str $filename, Pone::Node $node) {
    $!filename = $filename;
    self!compile($node);
}

method !compile(Pone::Node $node) {
    given $node {
    when Pone::Node::Stmts {
        $node.children.map({
            sprintf(qq!#line %d "%s"\n%s;\n!,
                .lineno, $!filename, self!compile($_))
        }).join("\n");
    }
    when Pone::Node::Funcall {
        my ($ident-node, @arg-nodes) = $node.children;
        my $ident = $ident-node.value;
        my $arg = @arg-nodes.map({ self!compile($_) }).join(", ");

        my $prefix = $!builtins{$ident} ?? "pone_builtin" !! "pone_user_func";
        sprintf('%s_%s(PONE_WORLD, %s)',
            $prefix, $ident, $arg);
    }
    when Pone::Node::If {
        my ($cond, $block, $else) = $node.children;
        my $s = "if (pone_so(";
        $s ~= self!compile($cond);
        $s ~= ')) {';
        $s ~= self!compile($block);
        $s ~= '}';
        if $else {
            $s ~= ' else {';
            $s ~= self!compile($else);
            $s ~= '}';
        }
        $s;
    }
    when Pone::Node::Block {
        # TODO: split SAVETMPS and lex scope
        my $s = '{' ~ "\n";
        $s ~= "pone_enter(PONE_WORLD);\n";
        $s ~= .children.map({self!compile($_)}).join("\n");
        $s ~= "pone_leave(PONE_WORLD);\n";
        $s ~= "}\n";
        $s;
    }

    when Pone::Node::Array {
        my $s = 'pone_new_ary(PONE_WORLD, ';
        $s ~= .children.elems;
        if .children.elems {
            $s ~= ",";
            $s ~= .children.map({ self!compile($_) }).join(",");
        }
        $s ~= ")";
        mortal($s);
    }

    when Pone::Node::Hash {
        my $s = 'pone_new_hash(PONE_WORLD, ';
        $s ~= .children.elems;
        if .children.elems {
            $s ~= ",";
            $s ~= .children.map({
                $_ ~~ Pone::Node::Pair or die "should not reach here";
                self!compile(.children[0]) ~ "," ~ self!compile(.children[1])
            }).join(",");
        }
        $s ~= ")";
        mortal($s);
    }

    when Pone::Node::Int {
        "pone_mortalize(PONE_WORLD, pone_new_int(PONE_WORLD, " ~ .value ~ "))";
    }
    when Pone::Node::Add {
        self!infix('pone_add', $_);
    }
    when Pone::Node::Subtract {
        self!infix('pone_subtract', $_);
    }
    when Pone::Node::Multiply {
        self!infix('pone_multiply', $_);
    }
    when Pone::Node::Assign {
        my $var = .children[0];
        if $var ~~ Pone::Node::My {
            $var = $var.children[0].value;
        }
        "pone_assign(PONE_WORLD, 0, \"$var\", " ~ self!compile(.children[1]) ~ ")";
    }
    when Pone::Node::Var {
        qq!pone_get_lex(PONE_WORLD, "{.value}")!;
    }
    when Pone::Node::True {
        "pone_true()";
    }
    when Pone::Node::False {
        "pone_false()";
    }
    when Pone::Node::Str {
        # TODO: freeze string literals
        my $s = .value;
        mortal(qq!pone_new_str(PONE_WORLD, "{escape-c-str($s)}", {$s.encode.bytes})!);
    }
    default {
        die "unknown node: {$node.WHAT.gist}";
    }
    }
}

