# NAME

Array - Built-in Array class.

# LITERAL

There is a literal to build an instance.

    [1,2,3]

# METHODS

### `Array#iterator()`

    my $iter = [1,2,3].iterator();

Get new instance of an iterator.

## `Array#elems() --` Int>

Get the number of elements.

## `Array#append($val)`

    my $a = [1,2,3];
    $a.append(4);
    say $a.join(","); # => 1,2,3,4

append an element to array tail.

## `Array#pop($val)`

    my $a = [1,2,3];
    say $a.pop(); # => 3

Pop element from an array.

NYI

## `Array#prepend($val)`

    my $a = [1,2,3];
    $a.prepend(0);
    say $a.join(','); # => 0,1,2,3

Prepend element to an array.

NYI

## `Array#shift($val)`

    my $a = [1,2,3];
    say $a.shift(); # => 1

Removes and returns the first item from the array. Fails for an empty arrays.

NYI

## `Array#end()`

Returns the index of the last element.

NYI

## `Array#join($separator)`

    say [1,2,3].join(','); # 1,2,3

Treats the elements of the list as strings, interleaves them with $separator and concatenates everything into a single string.

NYI
