#!/usr/bin/env perl6 -Ilib
use v6;

use lib 'lib';

use Test;
use Pone::Utils;

is escape-c-str("hoge"), 'hoge';
is escape-c-str(q!h"oge!), q!h\\"oge!;

done-testing;

