use . test;

like file.open("t/dat/hello.txt").slurp_rest, 'hello';
like file.open("t/dat/hello.txt").stat.size, 5;

done_testing();

