use . test;

class Foo {
    has $.year;

    sub set() {
        $.year = 2015;
    }

    sub next_year() {
        return $.year + 1;
    }
}

my $foo = bless(Foo);
$foo.set();
like $foo.next_year(), 2016;

done_testing();

