[![Build Status](https://travis-ci.org/tokuhirom/Pone.svg?branch=master)](https://travis-ci.org/tokuhirom/Pone)

The  ( д)  ﾟﾟPone Programming Language
=============================

There's more than one way to do it

SYNOPSIS
========

DESCRIPTION
===========

Pone is an Perl-ish interpreter. It has a Perl like syntax. This compiler compiles code into C.

FEATURES
========

  * Threading support
   * No global interpreter lock
  * No GC stop the world
   * GC working per thread.
  * Channel based thread model

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

## Is there a destructor(DESTROY)?

No. Destructor is not supported. Since we are using GC. You should use LEAVE block instead.
(Same as golang's defer)

AUTHOR
======

Tokuhiro Matsuno <tokuhirom@gmail.com>

COPYRIGHT AND LICENSE
=====================

    The MIT License (MIT)
    Copyright © 2015 Tokuhiro Matsuno, http://64p.org/ <tokuhirom@gmail.com>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the “Software”), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
    You have new mail.

