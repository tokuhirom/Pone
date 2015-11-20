#include "pone.h"
#include "pone_module.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

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

PONE_FUNC(meth_sock_close) {
    PONE_ARG("Socket#close", "");
    struct pone_sock* sock = pone_opaque_ptr(self);
    sock_close(sock);
    return pone_nil();
}

PONE_FUNC(meth_sock_write) {
    pone_val* buf;
    PONE_ARG("Socket#write", "o", &buf);
    assert(buf);

    struct pone_sock* sock = pone_opaque_ptr(self);
    ssize_t len = write(sock->fd, pone_str_ptr(buf), pone_str_len(buf));
    return pone_int_new(world, len);
}

PONE_FUNC(meth_sock_read) {
    pone_int_t length;
    PONE_ARG("Socket#read", "i", &length);

    char* buf = pone_malloc(world->universe, length);

    struct pone_sock* sock = pone_opaque_ptr(self);
    ssize_t len = read(sock->fd, buf, length);
    if (len >= 0) {
        return pone_bytes_new_allocd(world, buf, len);
    } else {
        pone_world_set_errno(world);
        return pone_nil();
    }
}

PONE_FUNC(meth_sock_accept) {
    PONE_ARG("Socket#accept", "");

    struct pone_sock* sock = pone_opaque_ptr(self);

    struct pone_sock* csock = pone_malloc(world->universe, sizeof(struct pone_sock));
    socklen_t addrlen = sizeof(struct sockaddr);
#ifdef HAVE_ACCEPT4
    csock->fd = accept4(sock->fd, &(csock->addr), &addrlen, SOCK_CLOEXEC);
#else
    csock->fd = accept(sock->fd, &(csock->addr), &addrlen);
#endif

    if (csock->fd != -1) {
#ifndef HAVE_ACCEPT4
        if (fcntl(csock->fd, F_SETFD, FD_CLOEXEC) == -1) {
            pone_world_set_errno(world);
            pone_free(world->universe, csock);
            return pone_nil();
        }
#endif
        pone_val* opaque = pone_opaque_new(world, csock, finalizer);
        pone_opaque_set_class(world, opaque, pone_what(world, self));
        return opaque;
    } else {
        // got error
        pone_world_set_errno(world);
        pone_free(world->universe, csock);
        return pone_nil();
    }

}

PONE_FUNC(meth_sock_connect) {
    const char* addr;
    const char* port;
    PONE_ARG("Socket#connect", "ss", &addr, &port);

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
#ifdef HAVE_SOCK_CLOEXEC
        rp->ai_socktype |= SOCK_CLOEXEC;
#endif
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) {
            pone_world_set_errno(world);
            continue;
        }
#ifndef SOCK_CLOEXEC
        if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
            pone_world_set_errno(world);
            continue;
        }
#endif

        if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;                  /* Success */
        }
        pone_world_set_errno(world);

        close(fd);
    }

    freeaddrinfo(result);           /* No longer needed */

    if (rp == NULL) {               /* No address succeeded */
        return pone_nil();
    }

    struct pone_sock* sock = pone_malloc(world->universe, sizeof(struct pone_sock));
    sock->fd = fd;

    pone_val* opaque = pone_opaque_new(world, sock, finalizer);
    pone_opaque_set_class(world, opaque, pone_get_lex(world, "klass"));
    return opaque;
}

PONE_FUNC(meth_sock_listen) {
    const char* addr;
    const char* port;
    pone_int_t backlog = SOMAXCONN;
    PONE_ARG("Socket#listen", "ss:i", &addr, &port, &backlog);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;          /* Any protocol */

    struct addrinfo *result, *rp;

    int s = getaddrinfo(addr, port, &hints, &result);
    if (s != 0) {
        pone_throw_str(world, "getaddrinfo: %s:%s: %s\n", addr, port, gai_strerror(s));
    }

    int fd = 0;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
#ifdef HAVE_SOCK_CLOEXEC
        rp->ai_socktype |= SOCK_CLOEXEC;
#endif
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) {
            pone_world_set_errno(world);
            continue;
        }
#ifndef SOCK_CLOEXEC
        if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
            pone_world_set_errno(world);
            continue;
        }
#endif

        if (bind(fd, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;                  /* Success */
        }
        pone_world_set_errno(world);

        close(fd);
    }

    freeaddrinfo(result);           /* No longer needed */

    if (rp == NULL) {               /* No address succeeded */
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
    pone_opaque_set_class(world, opaque, pone_get_lex(world, "klass"));
    return opaque;
}

void PONE_DLL_io_socket_inet(pone_world* world, pone_val* module) {
    // TODO setsockopt
    // TODO getsockopt

    pone_val* klass = pone_class_new(world, "Socket", strlen("Socket"));
    pone_add_method_c(world, klass, "accept", strlen("accept"), meth_sock_accept);
    pone_add_method_c(world, klass, "close", strlen("close"), meth_sock_close);
    pone_add_method_c(world, klass, "write", strlen("write"), meth_sock_write);
    pone_add_method_c(world, klass, "read", strlen("read"), meth_sock_read);
    pone_class_compose(world, klass);

    pone_module_put(world, module, "Socket", klass);

    {
        pone_val* code_listen = pone_code_new(world, meth_sock_listen);
        pone_code_bind(world, code_listen, "klass", klass);
        pone_module_put(world, module, "listen", code_listen);
    }
    {
        pone_val* code_connect = pone_code_new(world,  meth_sock_connect);
        pone_code_bind(world, code_connect, "klass", klass);
        pone_module_put(world, module, "connect", code_connect);
    }
}

