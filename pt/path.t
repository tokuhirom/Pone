use . test;

my $s = path.new("t/dat/hello.txt").stat;
ok $s;
like $s.size, 5;

done_testing();

