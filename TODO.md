TODO
====

## Concurrency

 * GVL is required for
   * lex operation
   * world operation
   * mutable objects
 * Channel


## Builtin functions

 * rand
 * open
 * `system` builtin function
 * chomp
 * chr
 * ord
 * hex
 * sprintf
 * rewrite printf
 * caller
 * __FILE__
 * __LINE__
 * getpid

## Builtin classes

 * File
 * IO::Socket::UNIX
 * `Proc` builtin class
 * Regex.quote
 * Str#index
 * Str#lc
 * Str#uc
 * Str#lcfirst
 * Str#ucfirst
 * Str#chars
 * Str#bytes
 * Str#split
 * Str#reverse
 * Array#reverse
 * Array#grep
 * Dir.open
 * File#close
 * File#say
 * File#print
 * File#readline
 * File#lines
 * Path::Dir
 * Path::File
 * Path::File#stat
 * IO::Socket::INET#setsockopt
 * IO::Socket::INET#getsockopt
 * Time
 * URI

## Basic features

 * `class A { }`
 * `class A is B { }`
 * file i/o
 * str methods
 * bytes literal/class
 * // operator
 * bit ops
 * -p
 * -lane
 * -i
 * `and`
 * `or`
 * `xor`
 * `&&`
 * `||`
 * `s///`
 * remove thread list in universe.
 * `while True { last; }`
 * `sub x() { for 1..10 { say 3; } }; say 5`

## Bindings

 * URI
 * HTTP::Parser
 * h2o
 * hiredis
 * libmysqlclient
 * FFI
 * CSV reader
 * JSON reader/writer

### GC

 * bitmap marking?
 * incremental GC?

### Syntax

 * Switch to C like operators

### Regexp

 * Use onigmo

## Known bugs

 * Exception from thread case segv.

