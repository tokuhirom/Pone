use strict;
use warnings;
use lib ' t/extlib/lib/perl5/';
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
my $chan = chan(1);
async(sub {
    my $sig = $chan.receive;
    say "GOT SIGNAL";
    exit();
});
say "hehhL";
signal.notify($chan, signal.SIGINT);
signal.kill($$, signal.SIGINT);
sleep 100
...
} else {
    waitpid $pid, 0;
    open my $fh, '<', $filename;
    my $res = do { local $/; <$fh> };
    like $res, qr/GOT SIGNAL/;
}

done_testing;

