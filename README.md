[![Stories in Ready](https://badge.waffle.io/tokuhirom/Pone.png?label=ready&title=Ready)](https://waffle.io/tokuhirom/Pone)
[![Build Status](https://travis-ci.org/tokuhirom/Pone.svg?branch=master)](https://travis-ci.org/tokuhirom/Pone)

The  ( д)  ﾟﾟPone Programming Language
=============================

There's more than one way to do it

SYNOPSIS
========

    use socket;
    use http/parser;

    my $chan = chan(9);

    # start worker thread.
    for 1..10 {
        async(sub {
            # TODO loop
            while True {
                my $csock = $chan.receive();
                my $buf;
                while True {
                    my $cbuf = $csock.read(1000);
                    if $cbuf.bytes > 0 {
                        $buf ~= $cbuf;
                        my $got = parser.parse_request($buf);
                        if $got[0] > 0 {
                            $csock.write("HTTP/1.0 200 OK\r\nContent-Length: 8\r\nConnection: close\r\n\r\nHello!!\n");
                            $csock.close();
                            last;
                        }
                    } else {
                        last;
                    }
                }
            }
        });
    }

    my $sock = socket.listen("127.0.0.1", 0) or die "listen: $!";
    say $sock.sockaddr;
    say $sock.sockport;

    while True {
        my $csock = $sock.accept();
        $chan.send($csock);
    }

DESCRIPTION
===========

Pone is an Perl-ish interpreter. It has a Perl like syntax. This compiler compiles code into C.

STATUS
======

Pone language is in pre-alpha state. Language design changes day-by-day.
I need your feedback.

FEATURES
========

* ** Threading support **

Pone includes threading support with shared nothing architecture.
It means there's no global interpreter lock in Pone interpreter. All threads run concurrently.

* ** No Global GC stop **

There's no global GC stop. Since each thread has own memory arena.
In TCP server, you can run GC when the thread doesn't get a request.

* ** Message passing model **

Pone is shared nothing architecture. Use message passing instead.
There's golang like channel for communcation.

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

  * True/False

There's boolean value!

builtin functions
=================

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

