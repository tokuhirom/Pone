# Signal

## DESCRIPTION

Package signal implements access to incoming signals.

## `signal.notify(Channel $channnel, Int $signal)`

Register signal handler.

When signal handler thread caught signal, signal handler thread sends signal number to `$channel`.

If `$channel`'s buffer doesn't have enough buffer space, notifier don't send signal to the channel.

## `signal.kill(Int $pid, Int $signal)`

Send `$signal` to `$pid`.

## `signal.SIGHUP`
## `signal.SIGINT`
## `signal.SIGQUIT`
## `signal.SIGILL`
## `signal.SIGABRT`
## `signal.SIGFPE`
## `signal.SIGKILL`
## `signal.SIGSEGV`
## `signal.SIGPIPE`
## `signal.SIGALRM`
## `signal.SIGTERM`
## `signal.SIGUSR1`
## `signal.SIGUSR2`
## `signal.SIGCHLD`
## `signal.SIGCONT`
## `signal.SIGSTOP`
## `signal.SIGTSTP`
## `signal.SIGTTIN`
## `signal.SIGTTOU`
## `signal.SIGBUS`
## `signal.SIGPOLL`
## `signal.SIGPROF`
## `signal.SIGSYS`
## `signal.SIGTRAP`
## `signal.SIGURG`
## `signal.SIGVTALRM`
## `signal.SIGXCPU`
## `signal.SIGXFSZ`

Get signal number.


