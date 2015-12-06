use . test;

ok <a b c> ~~ <a b c>;
ok(!(<a b c> ~~ <a c>));

like <c bb aa a b d 1 A Z>.sort, <1 A Z a aa b bb c d>;

done_testing();

