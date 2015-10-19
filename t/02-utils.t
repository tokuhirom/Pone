use v6;

use Test;
use Pone::Utils;

is escape-c-str("hoge"), 'hoge';
is escape-c-str("h'oge"), 'h\x27oge';
is escape-c-str("hおge"), 'h\u304Age';

done-testing;

