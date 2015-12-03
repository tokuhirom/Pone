use . test;

use math;

# abs
like(math.abs(-1), 1);
like(math.abs(-5.1), 5.1);

like math.PI.Str, /^3.14/;

done_testing();

