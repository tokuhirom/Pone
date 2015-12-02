use . test;

# (list_assignment (my (nop) (list (variable "$n") (variable "$y"))) (list (int 1) (int 2)))
my ($n, $y)=(1,2);

like $n, 1;
like $y, 2;

done_testing();

