use . test;

sub x() { nil }
my $n=0;
x() or $n=1;

like $n, 1;

done_testing();



