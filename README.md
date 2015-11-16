[![Build Status](https://travis-ci.org/tokuhirom/Pone.svg?branch=master)](https://travis-ci.org/tokuhirom/Pone)

NAME
====

Pone - Perl-ish minimal language

SYNOPSIS
========

DESCRIPTION
===========

Pone is an Perl-ish interpreter. It has a Perl like syntax. This compiler compiles code into C.

FEATURES
========

  * Threading support
   * No global interpreter lock

LITERALS
========

  * `0`, `1`, `100`

There is an integer literal.

  * `0.5`, `3.14`

There is a number literal.

  * `'hoge' `

There is single quotation string literal

  * `{ a = 3 }`

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

*Do not communicate by sharing memory; instead, share memory by communicating*.

 * Shared nothing architecture. Each threads communicate by Channels.
 * built-in Array/Hash is *not* thread safe.

There is no GVL in Pone. But there's some synchronization point.

 * Creating new thread.
  * Since Pone manages threads as linked-list.

And there's some limitations.

 * You can't assign variables that created by another thread.

FAQ
===

## Is there a thread.join equivalent?

No. You should use termination channel instead.

AUTHOR
======

Tokuhiro Matsuno <tokuhirom@gmail.com>

COPYRIGHT AND LICENSE
=====================

Copyright 2015 Tokuhiro Matsuno

This library is free software; you can redistribute it and/or modify it under the Artistic License 2.0.
