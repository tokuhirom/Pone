TODO
====

## Concurrency

 * GVL is required for
   * lex operation
   * world operation
   * mutable objects
 * Channel

## Basic features

 * `class A { }`
 * `class A is B { }`
 * `fork()`
 * socket i/o
 * file i/o
 * array methods
 * str methods
 * `system` builtin function
 * Implement `%*ENV` and deprecate `getenv()` builtin function
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
 * `Proc` builtin class
 * implement p5-ish built-in functions
 * remove thread list in universe.

## Bindings

 * h2o
 * hiredis
 * libmysqlclient
 * FFI

### GC

 * bitmap marking?
 * incremental GC?

### Syntax

 * Switch to C like operators

### Regexp

 * Use onigmo

## Known bugs

 * Exception from thread case segv.

