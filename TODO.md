TODO
====

## Builtin variables

done.

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
 * Implement socket.fdopen for server-starter
 * math module
   *         "atan2", "cos", "exp", "hex", "log", "oct", "rand",
         "sin", "sqrt", "srand"

## Builtin classes

 * `Socket#listen_un`
 * `Socket#connect_un`
 * `Proc` builtin class
 * Regex.quote
 * Str#index
 * Str#split
 * Str#reverse
 * Str#chomp
 * Array#reverse
 * Array#grep
 * Dir.open
 * dir.open('/etc') or `"/etc/".IO.dir?
 * File#say
 * File#readline
 * File#lines
 * Path::Dir
 * Path::File
 * IO::Socket::INET#setsockopt
 * IO::Socket::INET#getsockopt
 * Time#strftime
 * URI
 * `exec.which(path)` like golang's `exec.LookPath("fortune")`
 * argument validation for compiled code.

## Bundled packages

 * uri
  * uri.parse(Str $uri)
 * http/status
 * getopt

## Basic features

 * `class A does B { }`
 * `role A { }`
 * -p
 * -lane
 * -i
 * `s///`
 * named argument parameters.
 * `sub x(Code $code) { }` style argument validation.

## Bindings

 * hiredis
 * libmysqlclient
 * FFI
 * CSV reader
 * json.parse
 * text/markdown/howdown support
 * sqlite3

### GC

 * bitmap marking?
 * incremental GC?
 * generational GC

### Syntax

 * given-when

