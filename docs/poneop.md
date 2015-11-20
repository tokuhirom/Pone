# Operators

## Operator Precedence and Associativity

        left        terms and list operators (leftward)
        left        .
        nonassoc    ++ --
        right       **
        right       ! ~ \ and unary + and -
        left        =~ !~
        left        * / % x
        left        + - ~
        left        << >>
        nonassoc    named unary operators
        nonassoc    < > <= >= lt gt le ge
        nonassoc    == != <=> eq ne cmp ~~
        left        &
        left        | ^
        left        &&
        left        || //
        nonassoc    ..  ...
        right       ?:
        right       = += -= *= etc. last next redo
        left        , =>
        nonassoc    list operators (rightward)
        right       not
        left        and
        left        or

## Difference from perl5

 * no xor.
 * string concatnation operator is `~` instead of `.`.
 * method call operator is `.` instead of `->`.
 * no goto operator

