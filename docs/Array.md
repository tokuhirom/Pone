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

## `Array#push($val)`

    my $a = [1,2,3];
    $a.push(4);
    say $a.join(","); # => 1,2,3,4

Push an element to array tail.
