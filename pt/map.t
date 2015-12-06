use . test;

like {a => 3, b => 5}.keys.sort, ['a', 'b'];
like {a => 3, b => 5}.values.sort, [3,5];

# exists
like {a => 3}.exists('a'), True;
like {a => 3}.exists('b'), False;

done_testing();

