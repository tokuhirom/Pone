use . test;

like {a => 3, b => 5}.keys.sort, ['a', 'b'];
like {a => 3, b => 5}.values.sort, [3,5];

done_testing();

