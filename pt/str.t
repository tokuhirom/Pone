use . test;

like q!"!.chars, 1, "dq";
like "1+2=#{1+2}", "1+2=3", "interpolate";

like "0xff".Int(16), 255;
like "0331".Int(8), 217;

done_testing();

