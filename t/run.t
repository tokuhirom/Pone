use strict;
use warnings;
use v5.10.0;
use utf8;
use autodie;
use Test::More;
use Test::Base;
use File::Temp qw/tempfile/;

use Capture::Tiny ':all';

plan tests => 1*blocks;

run {
    my $block = shift;

    my ($fh, $filename) = tempfile();

    my $pid = fork() // die "fork failed: $!";
    if ($pid == 0) {
        open \*STDOUT, ">&=", fileno($fh) or die $!;
        open \*STDERR, ">&=", fileno($fh) or die $!;

        exec './bin/pone', '-e', $block->code;
        die;
    } else {
        waitpid $pid, 0;
        open my $fh, '<', $filename;
        my $res = do { local $/; <$fh> };
        like $res, qr/@{[ $block->re ]}/;
    }
    unlink $filename;
};

__END__

=== 
--- code: say "1000000000000000000000000000000".Int
--- re: over flow
