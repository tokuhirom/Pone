use v6;

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

    has @.children;
    has $.lineno is rw;
}

class Pone::Node::Stmts    is Pone::Node { }
class Pone::Node::If       is Pone::Node { }
class Pone::Node::Block    is Pone::Node { }
class Pone::Node::Funcall  is Pone::Node { }
class Pone::Node::Add      is Pone::Node { }
class Pone::Node::Subtract is Pone::Node { }
class Pone::Node::Multiply is Pone::Node { }
class Pone::Node::Assign   is Pone::Node { }
class Pone::Node::My       is Pone::Node { }

class Pone::Node::Array    is Pone::Node { }
class Pone::Node::Hash     is Pone::Node { }
class Pone::Node::Pair     is Pone::Node { }

class Pone::Node::Sub      is Pone::Node { }
class Pone::Node::Params   is Pone::Node { }

class Pone::Node::True  is Pone::Node { }
class Pone::Node::False is Pone::Node { }

class Pone::Node::Value is Pone::Node {
    has $.value is rw;

    multi method new(Int $value) {
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
class Pone::Node::Str   is Pone::Node::Value { }
class Pone::Node::Var   is Pone::Node::Value { }
class Pone::Node::Ident is Pone::Node::Value { }

