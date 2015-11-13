[![Build Status](https://travis-ci.org/tokuhirom/Pone.svg?branch=master)](https://travis-ci.org/tokuhirom/Pone)

NAME
====

Pone - Perl-ish minimal language

SYNOPSIS
========

DESCRIPTION
===========

Pone is a demo software for Perl6' grammar. It has a Perl like syntax. This compiler compiles code into C.

FEATURES
========

  * Reference counting GC

for stability.

  * Threading support

LITERALS
========

  * `0`, `1`, `100`

There is an integer literal.

  * `0.5`, `3.14`

There is a number literal.

  * `'hoge' `

There is single quotation string literal

  * `{ a =` 3 }>

hash literals.

builtin functions
=================

  * `elems`

Count elements.

  * `print(Str)`

print message.

  * `say(Str)`

print message with new line.

  * `getenv(Str)`

get environment variable

  * `time()`

Get current time in sec.

Concurrency
===========

 * built-in Array/Hash is *not* thread safe.

There is no GVL in Pone. But there's some synchronization point.

 * Creating new thread.
  * Since Pone manages threads as linked-list.

And thre's some limitations.

 * You can't assign variables that created by another thread.

TODO
====

  * user defined funciton
  * implement p5-ish built-in functions
  * Implement `%*ENV` and deprecate `getenv()` builtin function
  * `class Foo { }`
  * `class Foo is Bar { }`
  * socket support

AUTHOR
======

Tokuhiro Matsuno <tokuhirom@gmail.com>

COPYRIGHT AND LICENSE
=====================

Copyright 2015 Tokuhiro Matsuno

This library is free software; you can redistribute it and/or modify it under the Artistic License 2.0.
