use . test;

subtest 'string literal', sub {
    like q!"!.chars, 1, "dq";
    like "1+2=#{1+2}", "1+2=3", "interpolate";
    like "#", '#';
};

like "0xff".Int(16), 255;
like "0331".Int(8), 217;

done_testing();

