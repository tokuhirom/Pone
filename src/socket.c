#include "pone.h" /* PONE_INC */

static pone_val* meth_sock_connect(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);

    return pone_nil();
}

void pone_sock_init(pone_universe* universe) {
    assert(universe->class_io_socket_inet == NULL);

    universe->class_io_socket_inet = pone_class_new(universe, "IO::Socket::INET", strlen("IO::Socket::INET"));
    pone_class_push_parent(universe, universe->class_io_socket_inet, universe->class_any);
    pone_add_method_c(universe, universe->class_io_socket_inet, "connect", strlen("connect"), meth_sock_connect);
    pone_class_compose(universe, universe->class_io_socket_inet);
}
