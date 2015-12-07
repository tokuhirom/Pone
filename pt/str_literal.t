use . test;

like qq<FFF #{<a>.join("/")}>, "FFF a", 'nested string literal';

done_testing();

