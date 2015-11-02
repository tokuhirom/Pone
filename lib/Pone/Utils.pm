use v6;

unit class Pone::Utils;

sub escape-c-str(Str $s) is export {
    state %escape = (
        qq<\x07> => '\\a',
        qq<\x08> => '\\b',
        qq<\t> => '\\t',
        qq<\n> => '\\n',
        qq<\x0b> => '\\v',
        qq<\x0c> => '\\f',
        qq<\x0d> => '\\r',
        qq<\\> => '\\\\',
        qq<\"> => '\\"',
    );
    $s.subst(/(<[ \x07 \x08 \t \n \x0b \x0c \x0d \\ \"]>)/, -> $/ {
        $/[0].Str.perl.say;
        %escape{~$/[0]}
    }, :global);
}
