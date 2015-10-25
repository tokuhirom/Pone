use v6;

use Pone::Node;

unit class Pone::Compiler;

use Pone::Utils;

has $!filename;
has Set $.builtins = set(<print say dd abs elems getenv time>);
has @!subs;

sub mortal(Str $s) {
    "pone_mortalize(world, $s)"
}

sub inject-return(Pone::Node::Stmts $stmts) {
    if $stmts.children.elems > 0 {
        my $node = $stmts.children[*-1];
        if !$node.is-void {
            $stmts.children[*-1] = Pone::Node::Return.new($node);
        } else {
            given $node {
                when Pone::Node::Stmts {
                    inject-return($node);
                }
                when Pone::Node::Block {
                    inject-return($node.children[0]);
                }
            }
        }
    } else {
        # no statements in sub body
        $stmts.children.push(
            Pone::Node::Return.new(
                Pone::Node::Nil.new()
            )
        );
    }
    $stmts;
}

method !infix(Str $func, Pone::Node $node) {
    sprintf('%s(world, %s, %s)',
        $func,
        self!compile($node.children[0]),
        self!compile($node.children[1]));
}

method compile(Str $filename, Pone::Node $node) {
    my @*TMPS = 0;
    my @*SCOPE = 0;
    @!subs = ();
    $!filename = $filename;
    my $code = self!compile($node);
    return @!subs, $code;
}

method !so(Pone::Node $node) {
    if $node ~~ Pone::Node::True {
        "true";
    } else {
        "pone_so(" ~ self!compile($node) ~ ')';
    }
}

method !compile(Pone::Node $node) {
    given $node {
    when Pone::Node::Stmts {
        $node.children.map({
            my $s = '';
            if .lineno {
                $s ~= sprintf(qq!#line %d "%s"\n!,
                    .lineno, $!filename);
            }
            $s ~= self!compile($_) ~ ";\n";
            $s;
        }).join("\n");
    }
    when Pone::Node::Funcall {
        my ($ident-node, $args) = $node.children;
        my $ident = $ident-node.value;

        my $prefix = $!builtins{$ident} ?? "pone_builtin" !! "pone_user_func";
        my $s = $prefix;
        $s ~= "_";
        $s ~= $ident;
        $s ~= '(world';
        if $args {
            $s ~= ", " ~ self!compile($args);
        }
        $s ~= ')';
        $s;
    }
    when Pone::Node::Args {
        .children.map({self!compile($_)}).join(',');
    }
    when Pone::Node::If {
        my ($cond, $block, $else) = $node.children;
        my $s = "if (" ~ self!so($cond) ~ ') {';
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
        @*TMPS[*-1]++;
        $s ~= "pone_savetmps(world);\n";
        @*SCOPE[*-1]++;
        $s ~= "pone_push_scope(world);\n";
        $s ~= .children.map({self!compile($_)}).join("\n");
        @*SCOPE[*-1]++;
        $s ~= "pone_pop_scope(world);\n";
        @*TMPS[*-1]++;
        $s ~= "pone_freetmps(world);\n";
        $s ~= "}\n";
        $s;
    }

    when Pone::Node::Array {
        my $s = 'pone_new_ary(world, ';
        $s ~= .children.elems;
        if .children.elems {
            $s ~= ",";
            $s ~= .children.map({ self!compile($_) }).join(",");
        }
        $s ~= ")";
        mortal($s);
    }

    when Pone::Node::Hash {
        my $s = 'pone_new_hash(world, ';
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

    when Pone::Node::Sub {
        my $name = .children[0].value;
        my @vars = gather {
            if .children[1] {
                for 0..^(.children[1].children.elems) -> $i {
                    take "pone_val* arg$i";
                }
            }
        };
        my $s = '';
        $s ~= sprintf(qq!#line %d "%s"\n!, .lineno, $!filename);
        $s ~= 'pone_val* pone_user_func_';
        $s ~= $name;
        $s ~= '(pone_world* world';
        if @vars {
            $s ~= ',';
            $s ~= @vars.join(", ");
        }
        $s ~= ') {' ~ "\n";
        @*TMPS.push(0);
        $s ~= "pone_savetmps(world);\n";
        @*SCOPE.push(0);
        $s ~= "pone_push_scope(world);\n";
        # bind parameters to lexical variables
        if @vars {
            for 0..^(.children[1].children.elems) -> $i {
                my $var = .children[1].children[$i];
                $s ~= "pone_assign(world, 0, \"{$var.value}\", arg$i);\n";
            }
        }
        $s ~= self!compile(inject-return(.children[2]));
        @*SCOPE.pop();
        $s ~= "pone_pop_scope(world);\n";
        @*TMPS.pop();
        $s ~= "pone_freetmps(world);\n";
        $s ~= "\}\n";

        @!subs.push: $s;

        "/* <user func $name > */";
    }

    when Pone::Node::Int {
        "pone_mortalize(world, pone_new_int(world, " ~ .value ~ "))";
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
        "pone_assign(world, 0, \"$var\", " ~ self!compile(.children[1]) ~ ")";
    }
    when Pone::Node::Var {
        qq!pone_get_lex(world, "{.value}")!;
    }
    when Pone::Node::Return {
        if @*TMPS.elems == 1 {
            die "You can't return from outside of subroutine";
        }
        my $s = qq!do \{\n!;
        $s ~= sprintf(qq!pone_val* RETVAL=%s;\n!,  self!compile(.children[0]));
        $s ~= qq!pone_refcnt_inc(world, RETVAL);\n!;
        $s ~= qq!pone_freetmps(world);\n! for 0..@*TMPS[*-1];
        $s ~= qq!pone_pop_scope(world);\n! for 0..@*SCOPE[*-1];
        $s ~= qq!return pone_mortalize(world, RETVAL);\n!;
        $s ~= qq!\}while (0);\n!;
        $s;
    }
    when Pone::Node::True {
        "pone_true()";
    }
    when Pone::Node::False {
        "pone_false()";
    }
    when Pone::Node::Nil {
        "pone_nil()";
    }
    when Pone::Node::Str {
        # TODO: freeze string literals
        my $s = .value;
        mortal(qq!pone_new_str_const(world, "{escape-c-str($s)}", {$s.encode.bytes})!);
    }
    default {
        die "unknown node: {$node.WHAT.gist}";
    }
    }
}

