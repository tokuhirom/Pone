TODO
====

## Builtin variables

 * `$*ARGV`

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
 * LEAVE block
 * math module
   *         "atan2", "cos", "exp", "hex", "log", "oct", "rand",
         "sin", "sqrt", "srand"

## Builtin classes

 * `Socket#listen_un`
 * `Socket#connect_un`
 * `Proc` builtin class
 * Regex.quote
 * Str#index
 * Str#lc
 * Str#uc
 * Str#lcfirst
 * Str#ucfirst
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
 * Time#strftime
 * URI
 * `exec.which(path)` like golang's `exec.LookPath("fortune")`

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
 * -I lib

## Bindings

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
 * `if True { } else { }`

## Core

 * remove opaque's finalizer

