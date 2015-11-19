use strict;
use warnings;
use v5.10.0;
use utf8;
use autodie;
use Test::More;
use File::Temp qw/tempfile/;

use Capture::Tiny ':all';

my ($fh, $filename) = tempfile();

my $pid = fork() // die "fork failed: $!";
if ($pid == 0) {
    open \*STDOUT, ">&=", fileno($fh) or die $!;
    open \*STDERR, ">&=", fileno($fh) or die $!;

    exec './bin/pone', '-e', <<'...';
say "10000000000000000000".Int
...
    die;
} else {
    waitpid $pid, 0;
    open my $fh, '<', $filename;
    my $res = do { local $/; <$fh> };
    like $res, qr/over flow/;
}

done_testing;
