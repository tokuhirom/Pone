#include "pone.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

struct pone_sock {
    int fd;
    struct sockaddr addr;
};

static void sock_close(struct pone_sock* sock) {
    if (sock->fd) {
        close(sock->fd);
        sock->fd = 0;
    }
}

static void finalizer(pone_world* world, pone_val* val) {
    struct pone_sock* sock = pone_opaque_ptr(val);
    sock_close(sock);
    pone_free(world->universe, sock);
}

static pone_val* meth_sock_close(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);
    struct pone_sock* sock = pone_opaque_ptr(self);
    sock_close(sock);
    return pone_nil();
}

static pone_val* meth_sock_write(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);
    pone_val* buf = va_arg(args, pone_val*);
    assert(buf);

    struct pone_sock* sock = pone_opaque_ptr(self);
    ssize_t len = write(sock->fd, pone_str_ptr(buf), pone_str_len(buf));
    return pone_int_new(world, len);
}

static pone_val* meth_sock_read(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);
    pone_int_t length = pone_intify(world, va_arg(args, pone_val*));

    char* buf = pone_malloc(world->universe, length);

    struct pone_sock* sock = pone_opaque_ptr(self);
    ssize_t len = read(sock->fd, buf, length);
    if (len >= 0) {
        // TODO do not copy the buffer
        pone_val* retval = pone_str_new(world, buf, len);
        pone_free(world->universe, buf);
        return retval;
    } else {
        pone_world_set_errno(world);
        return pone_nil();
    }
}

static pone_val* meth_sock_accept(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);

    struct pone_sock* sock = pone_opaque_ptr(self);

    struct pone_sock* csock = pone_malloc(world->universe, sizeof(struct pone_sock));
    socklen_t addrlen = sizeof(struct sockaddr);
#ifdef HAVE_ACCEPT4
    csokc->fd = accept4(sock->fd, &(csock->addr), &addrlen, SOCK_CLOEXEC);
#else
    csock->fd = accept(sock->fd, &(csock->addr), &addrlen);
#endif

    if (csock->fd != -1) {
#ifdef HAVE_ACCEPT4
    {
        int yes = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            pone_world_set_errno(world);
            pone_free(world->universe, csock);
            return pone_nil();
        }
    }
#endif
        pone_val* opaque = pone_opaque_new(world, csock, finalizer);
        pone_opaque_set_class(world, opaque, world->universe->class_io_socket_inet);
        return opaque;
    } else {
        // got error
        pone_world_set_errno(world);
        pone_free(world->universe, csock);
        return pone_nil();
    }

}

static pone_val* meth_sock_connect(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 2);

    pone_val* addr_v = va_arg(args, pone_val*);
    const char* addr = pone_str_ptr(pone_str_c_str(world, addr_v));
    pone_val* port_v = va_arg(args, pone_val*);
    const char* port = pone_str_ptr(pone_str_c_str(world, pone_stringify(world, port_v)));

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;          /* Any protocol */

    struct addrinfo *result, *rp;
    printf("%s:%s\n", addr, port);

    int s = getaddrinfo(addr, port, &hints, &result);
    if (s != 0) {
        pone_throw_str(world, "getaddrinfo: %s:%s: %s\n", addr, port, gai_strerror(s));
    }

    int fd = 0;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype|SOCK_CLOEXEC, rp->ai_protocol);
        if (fd == -1)
            continue;

        if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1) {
            pone_world_set_errno(world);
            break;                  /* Success */
        }

        close(fd);
    }

    if (rp == NULL) {               /* No address succeeded */
        pone_world_set_errno(world);
        return pone_nil();
    }

    freeaddrinfo(result);           /* No longer needed */

    struct pone_sock* sock = pone_malloc(world->universe, sizeof(struct pone_sock));
    sock->fd = fd;

    pone_val* opaque = pone_opaque_new(world, sock, finalizer);
    pone_opaque_set_class(world, opaque, world->universe->class_io_socket_inet);
    return opaque;
}

static pone_val* meth_sock_listen(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 2 || n == 3);

    pone_val* addr_v = va_arg(args, pone_val*);
    const char* addr = pone_str_ptr(pone_str_c_str(world, addr_v));
    pone_val* port_v = va_arg(args, pone_val*);
    const char* port = pone_str_ptr(pone_str_c_str(world, pone_stringify(world, port_v)));
    pone_int_t backlog = n==3 ? pone_intify(world, port_v) : SOMAXCONN;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;          /* Any protocol */

    struct addrinfo *result, *rp;
    printf("%s:%s\n", addr, port);

    int s = getaddrinfo(addr, port, &hints, &result);
    if (s != 0) {
        pone_throw_str(world, "getaddrinfo: %s:%s: %s\n", addr, port, gai_strerror(s));
    }

    int fd = 0;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype|SOCK_CLOEXEC, rp->ai_protocol);
        if (fd == -1)
            continue;

        if (bind(fd, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;                  /* Success */
        }

        close(fd);
    }

    freeaddrinfo(result);           /* No longer needed */

    if (rp == NULL) {               /* No address succeeded */
        pone_world_set_errno(world);
        return pone_nil();
    }

    if (listen(fd, backlog) == -1) {
        pone_world_set_errno(world);
        return pone_nil();
    }

    // set SO_REUSEADDR by default.
    {
        int yes = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            pone_world_set_errno(world);
            return pone_nil();
        }
    }

    struct pone_sock* sock = pone_malloc(world->universe, sizeof(struct pone_sock));
    sock->fd = fd;

    pone_val* opaque = pone_opaque_new(world, sock, finalizer);
    pone_opaque_set_class(world, opaque, world->universe->class_io_socket_inet);
    return opaque;
}

void pone_sock_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_io_socket_inet == NULL);

    // TODO setsockopt
    // TODO getsockopt

    universe->class_io_socket_inet = pone_class_new(world, "IO::Socket::INET", strlen("IO::Socket::INET"));
    pone_class_push_parent(world, universe->class_io_socket_inet, universe->class_any);
    pone_add_method_c(world, universe->class_io_socket_inet, "listen", strlen("listen"), meth_sock_listen);
    pone_add_method_c(world, universe->class_io_socket_inet, "connect", strlen("connect"), meth_sock_connect);
    pone_add_method_c(world, universe->class_io_socket_inet, "accept", strlen("accept"), meth_sock_accept);
    pone_add_method_c(world, universe->class_io_socket_inet, "close", strlen("close"), meth_sock_close);
    pone_add_method_c(world, universe->class_io_socket_inet, "write", strlen("write"), meth_sock_write);
    pone_add_method_c(world, universe->class_io_socket_inet, "read", strlen("read"), meth_sock_read);
    pone_class_compose(world, universe->class_io_socket_inet);

    pone_universe_set_global(universe, "IO::Socket::INET", universe->class_io_socket_inet);
}

