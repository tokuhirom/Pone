use v6;

class Pone::Node::Nil { ... }
class Pone::Node::Str { ... }
class Pone::Node::Ident { ... }

class Pone::Node {
    multi method new(@children) {
        self.bless(:@children);
    }

    multi method new(Pone::Node $node) {
        my @children = $node;
        self.bless(:@children);
    }

    submethod BUILD(:@children) {
        @!children = @children;
    }

    method is-void() { True }

    has @.children;
    has $.lineno is rw;
}

class Pone::Node::Stmts    is Pone::Node { }
class Pone::Node::If       is Pone::Node { }
class Pone::Node::Block    is Pone::Node { }
class Pone::Node::Try      is Pone::Node { }
class Pone::Node::For      is Pone::Node { }
class Pone::Node::While    is Pone::Node { }

class Pone::Node::Funcall  is Pone::Node { }
class Pone::Node::Args     is Pone::Node { }
class Pone::Node::AtPos    is Pone::Node { }
class Pone::Node::MethodCall    is Pone::Node { }

class Pone::Node::Expr is Pone::Node {
    method is-void() { False }
}

class Pone::Node::Add      is Pone::Node::Expr { }
class Pone::Node::Subtract is Pone::Node::Expr { }
class Pone::Node::Multiply is Pone::Node::Expr { }
class Pone::Node::Divide   is Pone::Node::Expr { }
class Pone::Node::Mod      is Pone::Node::Expr { }
class Pone::Node::Assign   is Pone::Node { }
class Pone::Node::My       is Pone::Node { }
class Pone::Node::Return   is Pone::Node { }
class Pone::Node::Range    is Pone::Node { }

class Pone::Node::Term is Pone::Node {
    method is-void() { False }
}

class Pone::Node::Array    is Pone::Node::Term { }
class Pone::Node::Hash     is Pone::Node::Term { }
class Pone::Node::Pair     is Pone::Node::Term { }

class Pone::Node::Sub      is Pone::Node {
    method has-name() { .name.defined }
    method name() {
        given self.children[0] {
            when Pone::Node::Ident {
                .value
            }
            when Pone::Node::Nil {
                Nil;
            }
            default {
                die 'unknown node in Sub node: ' ~ .^name;
            }
        }
    }
}
class Pone::Node::Params   is Pone::Node { }

class Pone::Node::Nil    is Pone::Node::Term { }
class Pone::Node::True   is Pone::Node::Term { }
class Pone::Node::False  is Pone::Node::Term { }
class Pone::Node::ErrVar is Pone::Node::Term { }

class Pone::Node::Value is Pone::Node::Term {
    has $.value is rw;

    multi method new(Int $value) {
        self.bless()!initialize($value);
    }

    multi method new(Num $value) {
        self.bless()!initialize($value);
    }

    multi method new(Str $value) {
        self.bless()!initialize($value);
    }

    method !initialize($value) {
        $!value = $value;
        self;
    }
}
class Pone::Node::Int   is Pone::Node::Value { }
class Pone::Node::Num   is Pone::Node::Value { }
class Pone::Node::Str   is Pone::Node::Value { }
class Pone::Node::Var   is Pone::Node::Value { }
class Pone::Node::Ident is Pone::Node::Value { }

