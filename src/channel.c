#include "pone.h"
#include "pone_opaque.h"

/*
 * class Channel {
 *   has $!buffer-limit;
 *   has $!buffer;
 * }
 */

struct pone_chan {
    // using world is over spec. we should use pone_pool_t or something.
    pone_world* world;
    pone_int_t limit;
    pone_int_t num;
    pone_val* buffer;
    pthread_mutex_t mutex;
    pthread_cond_t recv_cond;
    pthread_cond_t send_cond;
};

void pone_chan_send(pone_world* world, pone_val* self, pone_val* val) {
    struct pone_chan* chan = pone_opaque_ptr(self);

    CHECK_PTHREAD(pthread_mutex_lock(&(chan->mutex)));
    while (chan->limit < chan->num) {
        // waiting channel buffer
        CHECK_PTHREAD(pthread_cond_wait(&(chan->recv_cond), &(chan->mutex)));
    }
    THREAD_TRACE("pone_chan_send chan:%p val:%p", chan, val);
    pone_ary_push(world->universe, chan->buffer, pone_val_copy(chan->world, val));
    chan->num++;
    CHECK_PTHREAD(pthread_cond_signal(&(chan->send_cond)));
    CHECK_PTHREAD(pthread_mutex_unlock(&(chan->mutex)));

    if (chan->world->gc_requested) {
        pone_gc_run(chan->world);
    }
}

/*
 * try to send value to the channel.
 * If the channel have enough buffer, send value and return true.
 * Return false otherwise.
 */
bool pone_chan_trysend(pone_world* world, pone_val* self, pone_val* val) {
    struct pone_chan* chan = pone_opaque_ptr(self);

    bool sent = false;
    CHECK_PTHREAD(pthread_mutex_lock(&(chan->mutex)));
    if (chan->limit > chan->num) {
        pone_ary_push(world->universe, chan->buffer, pone_val_copy(chan->world, val));
        chan->num++;
        CHECK_PTHREAD(pthread_cond_signal(&(chan->send_cond)));
        sent = true;
    }
    CHECK_PTHREAD(pthread_mutex_unlock(&(chan->mutex)));

    // TODO use CHANNEL_TRACE instead.
    THREAD_TRACE("sent channel item(trysend)");

    return sent;
}

pone_val* pone_chan_receive(pone_world* world, pone_val* self) {
    struct pone_chan* chan = pone_opaque_ptr(self);
    THREAD_TRACE("waiting channel item");

    CHECK_PTHREAD(pthread_mutex_lock(&(chan->mutex)));
    while (chan->num == 0) {
        CHECK_PTHREAD(pthread_cond_wait(&(chan->send_cond), &(chan->mutex)));
    }
    // copy value to reciever's world.
    pone_val* retval = pone_val_copy(world, pone_ary_shift(chan->world, chan->buffer));
    chan->num--;
    CHECK_PTHREAD(pthread_cond_signal(&(chan->recv_cond)));
    CHECK_PTHREAD(pthread_mutex_unlock(&(chan->mutex)));
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

static void chan_finalizer(pone_world* world, pone_val* val) {
    struct pone_chan* chan = pone_opaque_ptr(val);
    CHECK_PTHREAD(pthread_cond_destroy(&(chan->recv_cond)));
    CHECK_PTHREAD(pthread_cond_destroy(&(chan->send_cond)));
    CHECK_PTHREAD(pthread_mutex_destroy(&(chan->mutex)));
    pone_world_free(chan->world);
    pone_free(world->universe, chan);
}

pone_val* pone_chan_new(pone_world* world, pone_int_t limit) {
    struct pone_chan* chan = pone_malloc(world->universe, sizeof(struct pone_chan));
    chan->limit = limit;
    chan->world = pone_world_new(world->universe);
    chan->buffer = pone_ary_new(chan->world, 0);
    CHECK_PTHREAD(pthread_cond_init(&(chan->recv_cond), NULL));
    CHECK_PTHREAD(pthread_cond_init(&(chan->send_cond), NULL));
    CHECK_PTHREAD(pthread_mutex_init(&(chan->mutex), NULL));
    pone_val* obj = pone_opaque_new(world, chan, chan_finalizer);
    pone_opaque_set_class(chan->world, obj, world->universe->class_channel);
    return obj;
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
    pone_add_method_c(world, klass, "receive", strlen("receive"), meth_chan_receive);
    pone_add_method_c(world, klass, "send", strlen("send"), meth_chan_send);
    pone_class_compose(world, klass);

    universe->class_channel = klass;
    pone_universe_set_global(universe, "Channel", klass);
}
