use . test;

sub x { y() }
sub y() {
    like runtime.caller(0).subname, 'y';
    like runtime.caller(0).lineno, 4;
    like runtime.caller(1).subname, 'x';
    like runtime.caller(1).lineno, 3; # XXX
    like runtime.caller(2).subname, 'main';
    like runtime.caller(2).lineno, 12;
}
x();

done_testing()

