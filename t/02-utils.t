#!/usr/bin/env perl6 -Ilib
use v6;

use lib 'lib';

use Test;
use Pone::Utils;

is escape-c-str("hoge"), 'hoge';
is escape-c-str("h'oge"), "h\\'oge";
is escape-c-str("hおge"), 'h\u304Age';

done-testing;

