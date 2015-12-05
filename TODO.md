TODO
====

## Builtin variables

done.

## Builtin functions

 * `system` builtin function
 * chomp
 * chr
 * ord
 * sprintf
 * rewrite printf
 * __FILE__
 * __LINE__
 * LEAVE block

## Builtin classes

 * Implement socket.fdopen for server-starter
 * `socket#listen_un`
 * `socket#connect_un`
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
 * test.subtest

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

