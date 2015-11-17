#include "pone.h"

/*
 * class Channel {
 *   has $!buffer-limit;
 *   has $!buffer;
 * }
 */

void pone_chan_send(pone_world* world, pone_val* chan, pone_val* val) {
    pone_val* limit = pone_obj_get_ivar(world, chan, "$!buffer-limit");
    pone_val* buffer = pone_obj_get_ivar(world, chan, "$!buffer");
    pthread_cond_t* send_cond = (pthread_cond_t*)pone_intify(world, pone_obj_get_ivar(world, chan, "$!send-cond"));
    pthread_cond_t* recv_cond = (pthread_cond_t*)pone_intify(world, pone_obj_get_ivar(world, chan, "$!recv-cond"));
    pthread_mutex_t* mutex = (pthread_mutex_t*)pone_intify(world, pone_obj_get_ivar(world, chan, "$!mutex"));

    // if buffer is available
    GC_RD_LOCK(world->universe);
    CHECK_PTHREAD(pthread_mutex_lock(mutex));
    while (pone_intify(world, limit) < pone_ary_elems(buffer)) {
        CHECK_PTHREAD(pthread_cond_wait(recv_cond, mutex));
    }
    pone_ary_append(world->universe, buffer, val);
    CHECK_PTHREAD(pthread_cond_signal(send_cond));
    CHECK_PTHREAD(pthread_mutex_unlock(mutex));
    GC_UNLOCK(world->universe);
}

pone_val* pone_chan_receive(pone_world* world, pone_val* chan, pone_val* val) {
    pone_val* buffer = pone_obj_get_ivar(world, chan, "$!buffer");
    pthread_cond_t* send_cond = (pthread_cond_t*)pone_intify(world, pone_obj_get_ivar(world, chan, "$!send-cond"));
    pthread_cond_t* recv_cond = (pthread_cond_t*)pone_intify(world, pone_obj_get_ivar(world, chan, "$!recv-cond"));
    pthread_mutex_t* mutex = (pthread_mutex_t*)pone_intify(world, pone_obj_get_ivar(world, chan, "$!mutex"));

    GC_RD_LOCK(world->universe);
    CHECK_PTHREAD(pthread_mutex_lock(mutex));
    while (pone_ary_elems(buffer)==0) {
        CHECK_PTHREAD(pthread_cond_wait(send_cond, mutex));
    }
    pone_val* retval = pone_ary_shift(world, buffer);
    pone_save_tmp(world, retval);
    CHECK_PTHREAD(pthread_cond_signal(recv_cond));
    CHECK_PTHREAD(pthread_mutex_unlock(mutex));
    GC_UNLOCK(world->universe);

    return retval;
}

pone_val* pone_chan_new(pone_world* world, pone_int_t limit) {
    pone_val* obj = pone_obj_new(world, world->universe->class_channel);
    pone_obj_set_ivar(world, obj, "$!buffer-limit", pone_int_new(world, limit));
    pone_obj_set_ivar(world, obj, "$!buffer", pone_ary_new(world, 0));

    // TODO free these values in finalizer

    pthread_cond_t* send_cond = pone_malloc(world->universe, sizeof(pthread_cond_t));
    CHECK_PTHREAD(pthread_cond_init(send_cond, NULL));
    pone_obj_set_ivar(world, obj, "$!send-cond", pone_int_new(world, (pone_int_t)send_cond));

    pthread_cond_t* recv_cond = pone_malloc(world->universe, sizeof(pthread_cond_t));
    CHECK_PTHREAD(pthread_cond_init(recv_cond, NULL));
    pone_obj_set_ivar(world, obj, "$!recv-cond", pone_int_new(world, (pone_int_t)recv_cond));

    pthread_mutex_t* mutex = pone_malloc(world->universe, sizeof(pthread_mutex_t));
    CHECK_PTHREAD(pthread_mutex_init(mutex, NULL));
    pone_obj_set_ivar(world, obj, "$!mutex", pone_int_new(world, (pone_int_t)mutex));

    return obj;
}

static pone_val* meth_chan_new(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);

    pone_val*limit = va_arg(args, pone_val*);
    return pone_chan_new(world, pone_intify(world, limit));
}

/**

=head1 NAME

Channel - Thread-safe queue for sending values from producers to consumers

=head1 DESCRIPTION

A Channel is a thread-safe queue that helps you to send a series of objects from one or more producers to one or more consumers.

=cut

 */

static pone_val* meth_chan_receive(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);

    pone_val*val = va_arg(args, pone_val*);
    return pone_chan_receive(world, self, val);
}

static pone_val* meth_chan_send(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);

    pone_val*val = va_arg(args, pone_val*);
    pone_chan_send(world, self, val);
    return pone_nil();
}

void pone_channel_init(pone_world* world) {
    pone_universe* universe = world->universe;

    pone_val* klass = pone_class_new(world, "Channel", strlen("Channel"));
    pone_class_push_parent(world, klass, universe->class_any);
    pone_add_method_c(world, klass, "new", strlen("new"), meth_chan_new);
    pone_add_method_c(world, klass, "receive", strlen("receive"), meth_chan_receive);
    pone_add_method_c(world, klass, "send", strlen("send"), meth_chan_send);
    pone_class_compose(world, klass);

    universe->class_channel = klass;
    pone_universe_set_global(universe, "Channel", klass);
}

