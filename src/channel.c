#include "pone.h"
#include "pone_opaque.h"

/*
 * class Channel {
 *   has $!buffer-limit;
 *   has $!buffer;
 * }
 */

/*
 * Mark items in channel queue.
 * (This function don't mark channel itself)
 */
void pone_chan_mark_queue(pone_world* world, pone_val* chan) {
    THREAD_TRACE("pone_chan_mark_queue %p", chan);
    pthread_mutex_t* mutex = pone_opaque_ptr(pone_obj_get_ivar(world, chan, "$!mutex"));
    pone_val* buffer = pone_obj_get_ivar(world, chan, "$!buffer");
    CHECK_PTHREAD("pone_chan_mark_queue" && pthread_mutex_lock(mutex));
    for (pone_int_t i = 0; i < pone_ary_elems(buffer); ++i) {
        pone_gc_mark_value(pone_ary_at_pos(buffer, i));
    }
    CHECK_PTHREAD(pthread_mutex_unlock(mutex));
}

void pone_chan_send(pone_world* world, pone_val* chan, pone_val* val) {
    pone_val* limit_v = pone_obj_get_ivar(world, chan, "$!buffer-limit");
    pone_val* buffer = pone_obj_get_ivar(world, chan, "$!buffer");
    pthread_cond_t* send_cond = pone_opaque_ptr(pone_obj_get_ivar(world, chan, "$!send-cond"));
    pthread_cond_t* recv_cond = pone_opaque_ptr(pone_obj_get_ivar(world, chan, "$!recv-cond"));
    pthread_mutex_t* mutex = pone_opaque_ptr(pone_obj_get_ivar(world, chan, "$!mutex"));

    // if buffer is available
    pone_int_t limit = pone_intify(world, limit_v);
    CHECK_PTHREAD(pthread_mutex_lock(mutex));
    while (limit < pone_ary_elems(buffer)) {
        CHECK_PTHREAD(pthread_cond_wait(recv_cond, mutex));
    }
    THREAD_TRACE("pone_chan_send chan:%p val:%p", chan, val);
    pone_ary_push(world->universe, buffer, val);
    CHECK_PTHREAD(pthread_cond_signal(send_cond));
    CHECK_PTHREAD(pthread_mutex_unlock(mutex));
}

/*
 * try to send value to the channel.
 * If the channel have enough buffer, send value and return true.
 * Return false otherwise.
 */
bool pone_chan_trysend(pone_world* world, pone_val* chan, pone_val* val) {
    pone_val* limit_v = pone_obj_get_ivar(world, chan, "$!buffer-limit");
    pone_val* buffer = pone_obj_get_ivar(world, chan, "$!buffer");
    pthread_cond_t* send_cond = pone_opaque_ptr(pone_obj_get_ivar(world, chan, "$!send-cond"));
    pthread_mutex_t* mutex = pone_opaque_ptr(pone_obj_get_ivar(world, chan, "$!mutex"));

    bool sent = false;
    pone_int_t limit = pone_intify(world, limit_v);
    CHECK_PTHREAD(pthread_mutex_lock(mutex));
    if (limit > pone_ary_elems(buffer)) {
        pone_ary_push(world->universe, buffer, val);
        CHECK_PTHREAD(pthread_cond_signal(send_cond));
        sent = true;
    }
    CHECK_PTHREAD(pthread_mutex_unlock(mutex));

    THREAD_TRACE("sent channel item(trysend)");

    return sent;
}

pone_val* pone_chan_receive(pone_world* world, pone_val* chan) {
    THREAD_TRACE("waiting channel item");

    pone_val* buffer = pone_obj_get_ivar(world, chan, "$!buffer");
    pthread_cond_t* send_cond = pone_opaque_ptr(pone_obj_get_ivar(world, chan, "$!send-cond"));
    pthread_cond_t* recv_cond = pone_opaque_ptr(pone_obj_get_ivar(world, chan, "$!recv-cond"));
    pthread_mutex_t* mutex = pone_opaque_ptr(pone_obj_get_ivar(world, chan, "$!mutex"));

    CHECK_PTHREAD(pthread_mutex_lock(mutex));
    while (pone_ary_elems(buffer) == 0) {
        CHECK_PTHREAD(pthread_cond_wait(send_cond, mutex));
    }
    pone_val* retval = pone_val_copy(world, pone_ary_shift(world, buffer));
    pone_save_tmp(world, retval);
    CHECK_PTHREAD(pthread_cond_signal(recv_cond));
    CHECK_PTHREAD(pthread_mutex_unlock(mutex));
    THREAD_TRACE("got channel item");

    return retval;
}

static void finalize_mutex(pone_world* world, pone_val* val) {
    CHECK_PTHREAD(pthread_mutex_destroy(pone_opaque_ptr(val)));
    pone_free(world->universe, pone_opaque_ptr(val));
}

static void finalize_cond(pone_world* world, pone_val* val) {
    CHECK_PTHREAD(pthread_cond_destroy(pone_opaque_ptr(val)));
    pone_free(world->universe, pone_opaque_ptr(val));
}

pone_val* pone_chan_new(pone_world* world, pone_int_t limit) {
    pone_val* obj = pone_obj_new(world, world->universe->class_channel);
    pone_obj_set_ivar(world, obj, "$!buffer-limit", pone_int_new(world, limit));
    pone_obj_set_ivar(world, obj, "$!buffer", pone_ary_new(world, 0));

    pthread_cond_t* send_cond = pone_malloc(world->universe, sizeof(pthread_cond_t));
    CHECK_PTHREAD(pthread_cond_init(send_cond, NULL));
    pone_obj_set_ivar(world, obj, "$!send-cond", pone_opaque_new(world, send_cond, finalize_cond));

    pthread_cond_t* recv_cond = pone_malloc(world->universe, sizeof(pthread_cond_t));
    CHECK_PTHREAD(pthread_cond_init(recv_cond, NULL));
    pone_obj_set_ivar(world, obj, "$!recv-cond", pone_opaque_new(world, recv_cond, finalize_cond));

    pthread_mutex_t* mutex = pone_malloc(world->universe, sizeof(pthread_mutex_t));
    CHECK_PTHREAD(pthread_mutex_init(mutex, NULL));
    pone_obj_set_ivar(world, obj, "$!mutex", pone_opaque_new(world, mutex, finalize_mutex));

    return obj;
}

PONE_FUNC(meth_chan_new) {
    pone_int_t limit;
    PONE_ARG("Channel#new", ":i", &limit);

    return pone_chan_new(world, limit);
}

/**

=head1 NAME

Channel - Thread-safe queue for sending values from producers to consumers

=head1 DESCRIPTION

A Channel is a thread-safe queue that helps you to send a series of objects from one or more producers to one or more consumers.

=cut

 */

PONE_FUNC(meth_chan_receive) {
    PONE_ARG("Channel#receive", "");

    return pone_chan_receive(world, self);
}

PONE_FUNC(meth_chan_send) {
    pone_val* val;
    PONE_ARG("Channel#send", "o", &val);
    pone_chan_send(world, self, val);
    return pone_nil();
}

void pone_channel_init(pone_world* world) {
    pone_universe* universe = world->universe;

    pone_val* klass = pone_class_new(world, "Channel", strlen("Channel"));
    pone_add_method_c(world, klass, "new", strlen("new"), meth_chan_new);
    pone_add_method_c(world, klass, "receive", strlen("receive"), meth_chan_receive);
    pone_add_method_c(world, klass, "send", strlen("send"), meth_chan_send);
    pone_class_compose(world, klass);

    universe->class_channel = klass;
    pone_universe_set_global(universe, "Channel", klass);
}
