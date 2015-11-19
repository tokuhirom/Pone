# NAME

Mu - 無

# DESCRIPTION

The root of the Perl 6 type hierarchy. For the origin of the name, see http://en.wikipedia.org/wiki/Mu\_%28negative%29. One can also say that there are many undefined values in Perl 6, and Mu is the most undefined value.

Note that most classes do not derive from Mu directly, but rather from Any.

# METHODS

### `Mu#say()`

    [1,2,3].say();

Prints value to $\*OUT after stringification using .gist method with newline at end.

### `Mu#Str()`

Returns a string representation of the invocant, intended to be machine readable. Method Str warns on type objects, and produces the empty string.

# POD ERRORS

Hey! **The above document had some coding errors, which are explained below:**

- Around line 7:

    Non-ASCII character seen before =encoding in '無'. Assuming UTF-8
