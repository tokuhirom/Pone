# socket module

## DESCRIPTION

socket is a class for BSD Socket.

This class calls syscall. These method sets `$!` if syscall returns an error,

Pone supports IPv4 and IPv6.

## METHODS

### connect

    use socket;
    my $sock = socket.connect('localhost', 80);

This method calls `socket(2)`, `getaddrinfo(3)`, `connect(2)`.

pone sets `SOCK_CLOEXEC` flag automatically.

### `socket.listen($host, $port[, $backlog])`

    use socket;
    my $sock = socket.listen('localhost', 80);

Create new socket and start listen.

Default `$backlog` size is `SO_MAXCONN`.

pone sets `SO_REUSEADDR`, `SOCK_CLOEXEC` flag automatically.

### `my $buf = $sock.read($buflen)`

Read bytes from the socket.

### `$sock.write($buf)`

Write bytes to the socket.

### `$sock.close()`

Close the socket.

### `$sock.accept()`

Accept new connection.

pone sets `SOCK_CLOEXEC` flag automatically.

### `$sock.sockaddr()`

Get socket address.

### `$sock.sockport()`

Get socket port.

### `$sock.peeraddr() --> Str`

Returns the address of the peer connected to the socket.

### `$sock.peerport() --> Int`

Returns the port of the peer connected to the socket.


