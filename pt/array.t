use . test;

ok <a b c> ~~ <a b c>;
ok(!(<a b c> ~~ <a c>));

done_testing();

