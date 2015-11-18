# Signal

## DESCRIPTION

Package signal implements access to incoming signals.

## `Signal.notify(Channel $channnel, Int $signal)`

Register signal handler.

When signal handler thread caught signal, signal handler thread sends signal number to `$channel`.

If `$channel`'s buffer doesn't have enough buffer space, notifier don't send signal to the channel.

## `Signal.SIGINT`

Get signal number.


