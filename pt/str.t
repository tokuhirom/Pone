use . test;

like q!"!.chars, 1, "dq";
like "1+2=#{1+2}", "1+2=3", "interpolate";

done_testing();

