# Signal

## DESCRIPTION

Package signal implements access to incoming signals.

## `Signal.notify(Channel $channnel, Int $signal)`

Register signal handler.

When signal handler thread caught signal, signal handler thread sends signal number to `$channel`.

If `$channel`'s buffer doesn't have enough buffer space, notifier don't send signal to the channel.

## `Signal.SIGHUP`
## `Signal.SIGINT`
## `Signal.SIGQUIT`
## `Signal.SIGILL`
## `Signal.SIGABRT`
## `Signal.SIGFPE`
## `Signal.SIGKILL`
## `Signal.SIGSEGV`
## `Signal.SIGPIPE`
## `Signal.SIGALRM`
## `Signal.SIGTERM`
## `Signal.SIGUSR1`
## `Signal.SIGUSR2`
## `Signal.SIGCHLD`
## `Signal.SIGCONT`
## `Signal.SIGSTOP`
## `Signal.SIGTSTP`
## `Signal.SIGTTIN`
## `Signal.SIGTTOU`
## `Signal.SIGBUS`
## `Signal.SIGPOLL`
## `Signal.SIGPROF`
## `Signal.SIGSYS`
## `Signal.SIGTRAP`
## `Signal.SIGURG`
## `Signal.SIGVTALRM`
## `Signal.SIGXCPU`
## `Signal.SIGXFSZ`

Get signal number.


