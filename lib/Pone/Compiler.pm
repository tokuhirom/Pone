use v6;

use Pone::Node;

unit class Pone::Compiler;

use Pone::Utils;

has $!filename;
has Set $.builtins = set(<print say dd abs elems getenv time signal sleep die>);
has %.constants = (
    SIGINT => 2, # SIGINT should be enum
);
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
    mortal(sprintf('%s(world, %s, %s)',
        $func,
        self!compile($node.children[0]),
        self!compile($node.children[1])));
}

method compile(Str $filename, Pone::Node $node) {
    my @*TMPS = 0;
    my @*SCOPE = 0;
    my $*ANONSUBNO = 0;
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
            $s ~= "pone_signal_handle(world);\n";
            $s;
        }).join("\n");
    }
    when Pone::Node::Ident {
        if %!constants{.value}:exists {
            "pone_mortalize(world, pone_int_new(world->universe, {%!constants{.value}}))";
        } else {
            die "Unknown variable '{.value}'";
        }
    }
    when Pone::Node::Try {
        my $sub = Pone::Node::Sub.new([
            Pone::Node::Nil.new(), # name
            Pone::Node::Params.new(),
            .children[0]
        ]);
        $sub.lineno = .lineno;
        my $body = self!compile($sub);
        "pone_mortalize(world, pone_try(world, $body))"
    }
    when Pone::Node::While {
        my $term = .children[0];
        my $stmts = .children[1];
        q:to/EOD/.subst(/'<%=' (.*?)  '%>'/, { EVAL $0 }, :global);
        while (pone_so(<%= self!compile($term) %>)) {
            <%= self!compile($stmts) %>
        }
        EOD
    }
    when Pone::Node::For {
        my $obj = .children[0];
        my $stmts = .children[1];
        q:to/EOD/.subst(/'<%=' (.*?)  '%>'/, { EVAL $0 }, :global);
        {
            pone_val* iter = pone_mortalize(world, pone_iter_init(world, <%= self!compile($obj) %>));
            while (true) {
                pone_val* next = pone_mortalize(world, pone_iter_next(world, iter));
                if (next == world->universe->instance_iteration_end) {
                    break;
                }
                pone_assign(world, 0, "$_", next);
                <%= self!compile($stmts) %>
            }
        }
        EOD
    }
    when Pone::Node::ErrVar {
        'pone_errvar(world)'
    }
    when Pone::Node::AtPos {
        my $obj = self!compile(.children[0]);
        my $pos = self!compile(.children[1]);
        "pone_at_pos(world, $obj, $pos)";
    }
    when Pone::Node::Funcall {
        my ($func-node, $args) = $node.children;

        # builtin function call
        if $func-node ~~ Pone::Node::Ident {
            my $ident = $func-node.value;
            if $!builtins{$ident} {
                my $s = "pone_builtin_{$ident}\(world";
                if $args {
                    $s ~= ", " ~ self!compile($args);
                }
                $s ~= ')';
                return $s;
            }
        }

        my $funcname = do given $func-node {
            when Pone::Node::Ident {
                '&' ~ .value
            }
            when Pone::Node::Var {
                .value
            }
            default {
                die "invalid node for function call";
            }
        };

        my $s = qq!pone_code_call(world, pone_get_lex(world, "{$funcname}"), pone_nil(), !;
        if $args && $args.children.elems > 0 {
            my $argcnt = $args.children.elems;
            $s ~= "$argcnt, " ~ self!compile($args);
        } else {
            $s ~= "0";
        }
        $s ~= ")";
        $s;
    }
    when Pone::Node::MethodCall {
        my $obj = self!compile(.children[0]);
        my $ident = .children[1];
        my $args = self!compile(.children[2]);
        my $elems = .children[2].children.elems;

        if $ident ~~ Pone::Node::Ident {
            my $s = qq!pone_call_method(world, $obj, "{$ident.value}", {$elems}!;
            if $elems > 0 {
                $s ~= ", $args";
            }
            $s ~= ")";
            $s;
        } else {
            die "NYI { $ident.WHAT.Str }"
        }
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
        my $s = 'pone_ary_new(world->universe, ';
        $s ~= .children.elems;
        if .children.elems {
            $s ~= ",";
            $s ~= .children.map({ self!compile($_) }).join(",");
        }
        $s ~= ")";
        mortal($s);
    }

    when Pone::Node::Hash {
        my $s = 'pone_hash_new(world->universe, ';
        $s ~= .children.elems;
        if .children.elems {
            $s ~= ",";
            $s ~= .children.map({
                $_ ~~ Pone::Node::Pair or die "should not reach here";
                my $key = .children[0];
                if $key ~~ Pone::Node::Ident {
                    $key = Pone::Node::Str.new($key.value);
                }
                self!compile($key) ~ "," ~ self!compile(.children[1])
            }).join(",");
        }
        $s ~= ")";
        mortal($s);
    }

    when Pone::Node::Sub {
        my $name = .name // 'anon_' ~ $*ANONSUBNO++;
        my $argcnt = .children[1] ?? .children[1].children.elems !! 0;
        my $s = '';
        $s ~= sprintf(qq!#line %d "%s"\n!, .lineno, $!filename);
        $s ~= 'pone_val* pone_user_func_';
        $s ~= $name;
        $s ~= '(pone_world* world, pone_val* self, int n, va_list args) {' ~ "\n";
        $s ~= "assert(n==$argcnt);\n";
        @*TMPS.push(0);
        $s ~= "pone_savetmps(world);\n";
        @*SCOPE.push(0);
        $s ~= "pone_push_scope(world);\n";
        # bind parameters to lexical variables
        if $argcnt > 0 {
            if .children[1] {
                for 0..^(.children[1].children.elems) -> $i {
                    my $var = .children[1].children[$i];
                    $s ~= "pone_assign(world, 0, \"{$var.value}\", va_arg(args, pone_val*));\n";
                }
            }
        }
        $s ~= self!compile(inject-return(.children[2]));
        @*SCOPE.pop();
        $s ~= "pone_pop_scope(world);\n";
        @*TMPS.pop();
        $s ~= "pone_freetmps(world);\n";
        $s ~= "    return pone_nil();\n";
        $s ~= "\}\n";

        @!subs.push: $s;

        if .name {
            qq!pone_assign(world, 0, "&{$name}", pone_mortalize(world, pone_code_new(world, pone_user_func_{$name})))!;
        } else {
            qq!pone_mortalize(world, pone_code_new(world, pone_user_func_{$name}))!;
        }
    }

    when Pone::Node::Int {
        "pone_mortalize(world, pone_int_new(world->universe, " ~ .value ~ "))";
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
    when Pone::Node::Divide {
        self!infix('pone_divide', $_);
    }
    when Pone::Node::Mod {
        self!infix('pone_mod', $_);
    }
    when Pone::Node::Assign {
        my $var = .children[0];
        given $var {
            when Pone::Node::My {
                $var = $var.children[0].value;
            }
            when Pone::Node::Var {
                $var = $var.value;
            }
            default {
                die "unknown node on assignment";
            }
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
        sprintf(qq!return %s;\n!,
            self!compile(.children[0]));
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
        my $s = .value;
        mortal(qq!pone_str_new_const(world->universe, "{escape-c-str($s)}", {$s.encode.bytes})!);
    }
    default {
        die "unknown node: {$node.WHAT.gist}";
    }
    }
}

