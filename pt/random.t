use . test;

my $h = {};
for 1..1000 {
    my $i = random.rand_int(0,10);
    $h{"$i"} = 1;
}

for 0..10 {
    like $h{"$_"}, 1;
}

done_testing();
