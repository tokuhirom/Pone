# Channel

A Channel is a thread-safe queue that helps you to send a series of objects from one or more producers to one or more consumers.

## `sub chan(Int $buffer-length)`

    my $chan = chan(1024);

`chan()` creates new Channel with `$buffer-length` length buffer.

## `method receive()`

Receive value from the channel. This method blocks until other thread send value to this channel.

### Returns

Received value.

### Exception

`receive` throws X::Channel::ReceiveOnClosed exception if channel was closed.

## `method send($var)`

Send `$var` to receiver thread. If channel's buffer is reached to `$buffer-length`, this method waits until other thread receive the value.

### Returns

Nil

### Exception

`receive` throws X::Channel::SendOnClosed exception if channel was closed.

## `method close()`

Close the channel.

### Returns

Nil

## `method closed()`

Check the thread's status.

### Returns

True if the channel was closed. False otherwise.

