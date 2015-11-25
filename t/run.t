use strict;
use warnings;
use lib ' t/extlib/lib/perl5/';
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
        note $res;
        if ($block->text) {
            eval $block->test;
            die $@ if $@;
        } else {
            like $res, qr/@{[ $block->re ]}/;
        }
    }
    unlink $filename;
};

__END__

=== 
--- code: say "1000000000000000000000000000000".Int
--- re: over flow

=== 
--- code: say 1000000000000000000000000000000
--- re: overflow

=== 
--- code: say getcwd();
--- test
use Cwd;
my $wd = Cwd::getcwd();
note $wd;
like $res, qr/$wd/;

===
--- code: say $PONE_VERSION;
--- re: alpha

===
--- code: say os.is_win;
--- re: True|False

===
--- code: say 3.hoge
--- re: Method 'hoge' not found for invocant of class 'Int'

===
--- code: say localtime()
--- re: \d\d\d\d-\d\d-\d\dT\d\d:\d\d:\d\d[+-]\d\d\d\d

===
--- code: say gmtime()
--- re: \d\d\d\d-\d\d-\d\dT\d\d:\d\d:\d\d[+-]\d\d\d\d

===
--- code: say tmpfile().path()
--- re: pone_......

===
--- code: say tmpfile().file()
--- re: FILE.*

===
--- code: $*INC.unshift("t/lib"); use hoge hoge; say hoge.x()
--- re: hogehoge

