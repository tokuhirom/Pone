use v6;

unit class Pone::Utils;

sub escape-c-str(Str $s) is export {
    $s.subst(/(<-[A..Z a..z 0..9 _ - ]>)/, {
        my $c = ord($0);
        if $c < 0x100 {
            sprintf("\\x%X", ord($0));
        } else {
            sprintf("\\u%X", ord($0));
        }
    }, :global);
}
