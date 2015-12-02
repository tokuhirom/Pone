use . test;

sub x($n, $m=3) { like $m, 3; }
x(5)

sub y($n, $m=3) { like $m, 4; }
y(5,4)

done_testing();

