# NAME

Array - Built-in Array class.

# LITERAL

There is a literal to build an instance.

    [1,2,3]

# METHODS

## `Array#size() --> Int`

Get the number of elements.

## `Array#push($val)`

    my $a = [1,2,3];
    $a.push(4);
    say $a.join(","); # => 1,2,3,4

append an element to array tail.

## `Array#pop($val)`

    my $a = [1,2,3];
    say $a.pop(); # => 3

Pop element from an array.

## `Array#unshift($val)`

    my $a = [1,2,3];
    $a.unshift(0);
    say $a.join(','); # => 0,1,2,3

Prepend element to an array.

## `Array#shift($val)`

    my $a = [1,2,3];
    say $a.shift(); # => 1

Removes and returns the first item from the array. Fails for an empty arrays.

## `Array#join($separator)`

    say [1,2,3].join(','); # 1,2,3

Treats the elements of the list as strings, interleaves them with $separator and concatenates everything into a single string.

