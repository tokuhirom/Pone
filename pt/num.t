use . test;


like(0.2+0.1, /^0.3/);
like(0.2-0.1, /^0.1/);
like(0.2*10, 2);
like(0.2/10, /^0.02/);

like NAN, Num;
like INF, Num;

done_testing();

