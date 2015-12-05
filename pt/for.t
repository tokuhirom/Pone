use . test;

my $h = { };
{
    my $i=1;
    $h{"$i"} = 5963;
}
like $h<1>, 5963;

done_testing();
