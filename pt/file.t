use . test;

like file.open("t/dat/hello.txt").slurp_rest, 'hello';

done_testing();

