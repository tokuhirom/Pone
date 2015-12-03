use . test;

use math;

# abs
like(math.abs(-1), 1);
like(math.abs(-5.1), 5.1);

like math.PI.Str, /^3.14/;
like math.atan2(1,0), /^1.57/;
like math.cos(1), /^0.54/, 'cos';
like math.cos(math.PI), /^-1/, 'cos(pi)';
ok math.cos(math.PI/2) < 0.01;
like math.exp(0), 1;
like math.exp(1), /^2.71/, 'exp(1)';

like math.log(0), -INF;
like math.log(1), 0;
like math.log(4), /^1.38/;

like math.sin(0), 0;
like math.sin(1), /^0.84/;
like math.sin(2), /^0.90/;

like math.sqrt(2), /^1.414/;
like math.sqrt(3), /^1.732/;
like math.sqrt(4), 2;

done_testing();

