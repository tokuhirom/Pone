TODO
====

## Builtin functions

 * rand
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
 * gmtime
 * localtime
 * LEAVE block
 *         "abs", "atan2", "cos", "exp", "hex", "int", "log", "oct", "rand",
         "sin", "sqrt", "srand"

## Builtin classes

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
 * File#say
 * File#readline
 * File#lines
 * Path::Dir
 * Path::File
 * Path::File#stat
 * IO::Socket::INET#setsockopt
 * IO::Socket::INET#getsockopt
 * Time
 * Time#strftime
 * URI

## Basic features

 * `class A { }`
 * `class A does B { }`
 * `role A { }`
 * str methods
 * -p
 * -lane
 * -i
 * `s///`
 * `"hoge"[0]`
 * `while True { last; }`
 * `sub x() { for 1..10 { say 3; } }; say 5`

## Bindings

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
 * generational GC

### Syntax

 * given-when

## Core

 * remove opaque's finalizer

