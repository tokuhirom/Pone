export PONE_INCLUDE=include/
prove -r --exec './bin/pone' pt/
prove -I t/extlib/lib/perl5/ -r t xt
