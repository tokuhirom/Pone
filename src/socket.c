#include "pone.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

static pone_val* meth_sock_connect(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 2);

    pone_val* addr_v = va_arg(args, pone_val*);
    const char* addr = pone_str_ptr(pone_str_c_str(world, addr_v));
    pone_val* port_v = va_arg(args, pone_val*);
    const char* port = pone_str_ptr(pone_str_c_str(world, pone_stringify(world, port_v)));

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    struct addrinfo *result, *rp;

    int s = getaddrinfo(addr, port, &hints, &result);
    if (s != 0) {
        pone_throw_str(world, "getaddrinfo: %s:%s: %s\n", addr, port, gai_strerror(s));
    }

    int sfd;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */

        close(sfd);
    }

    if (rp == NULL) {               /* No address succeeded */
        // TODO throw exception
        pone_throw_str(world, "Could not connect: %s:%s\n", addr, port);
    }

    freeaddrinfo(result);           /* No longer needed */

    // TODO return socket object
    return pone_nil();
}

void pone_sock_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_io_socket_inet == NULL);

    universe->class_io_socket_inet = pone_class_new(world, "IO::Socket::INET", strlen("IO::Socket::INET"));
    pone_class_push_parent(world, universe->class_io_socket_inet, universe->class_any);
    pone_add_method_c(world, universe->class_io_socket_inet, "connect", strlen("connect"), meth_sock_connect);
    pone_class_compose(world, universe->class_io_socket_inet);

    pone_universe_set_global(universe, "IO::Socket::INET", universe->class_io_socket_inet);
}

