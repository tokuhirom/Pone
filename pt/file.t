use . test;

like file.open("t/dat/hello.txt").slurp_rest, 'hello';
like file.open("t/dat/hello.txt").stat.size, 5;

{
    my $fname="t/dat/lorem-ipsum.txt";
    my $s=0;
    for file.open($fname).lines { $s += $_.bytes };
    like $s, 2997;
}

done_testing();

