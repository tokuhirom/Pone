use . test;

class Foo {
    has $.year;

    sub set() {
        $.year = 2015;
    }

    sub next_year() {
        return $.year + 1;
    }

    sub m() {
        return $.m;
    }
}

my $foo = bless(Foo, { m => 5963 });
$foo.set();
like $foo.next_year(), 2016;
like $foo.m(), 5963;

done_testing();

