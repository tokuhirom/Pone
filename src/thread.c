#include "pone.h"
#include <errno.h>
#include <signal.h>

static void* thread_start(void* p) {
    THREAD_TRACE("NEW %lx", pthread_self());

    pone_world* world = (pone_world*)p;
    assert(world->universe);

    // mask all signals
    sigset_t set;
    sigfillset(&set);
    CHECK_PTHREAD(pthread_sigmask(SIG_BLOCK, &set, NULL));

    world->err_handler_lexs[0] = world->lex;
    if (setjmp(world->err_handlers[0])) {
        pone_universe_default_err_handler(world);
    } else {
        CHECK_PTHREAD(pthread_mutex_lock(&(world->mutex)));
        while (true) {
            while (!world->code) {
                CHECK_PTHREAD(pthread_cond_wait(&(world->cond), &(world->mutex)));
            }

            pone_push_scope(world);

            pone_val* code = world->code;
            assert(pone_type(code) == PONE_CODE);
            (void) pone_code_call(world, code, pone_nil(), 0);

            pone_universe* universe = world->universe;
            // move the world to free list
            pone_world_release(world);

            // tell thread termination to pone_universe_wait_threads.
            CHECK_PTHREAD(pthread_mutex_lock(&(universe->worker_fin_cond_mutex)));
            CHECK_PTHREAD(pthread_cond_signal(&(universe->worker_fin_cond)));
            CHECK_PTHREAD(pthread_mutex_unlock(&(universe->worker_fin_cond_mutex)));
        }
    }

    return NULL;
}

void pone_thread_start(pone_universe* universe, pone_val* code) {
    assert(pone_type(code) == PONE_CODE);

    WORLD_TRACE("pone_thread_start");

    // find waiting thread.
    {
        pone_world* world = universe->worker_worlds;
        while (world) {
            if (!world->code) {
                if (pthread_mutex_trylock(&(world->mutex)) == 0) {
                    // got mutex lock
                    WORLD_TRACE("Reuse %p", world);
                    world->code = code;
                    CHECK_PTHREAD(pthread_cond_signal(&(world->cond)));
                    CHECK_PTHREAD(pthread_mutex_unlock(&(world->mutex)));
                    return;
                }
            }
            world = world->next;
        }
    }

    // There's no waiting thread. We'll create new thread.

    CHECK_PTHREAD(pthread_mutex_lock(&(universe->worker_worlds_mutex)));
    pone_world* new_world = pone_world_new(universe);
    new_world->code = code;
    if (universe->worker_worlds) {
        new_world->next = universe->worker_worlds;
        universe->worker_worlds = new_world->next;
    } else {
        universe->worker_worlds = new_world;
    }
    CHECK_PTHREAD(pthread_mutex_unlock(&(universe->worker_worlds_mutex)));

    CHECK_PTHREAD(pthread_create(&(new_world->thread_id), NULL, &thread_start, new_world));
}

static pone_val* meth_thread_start(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);
    assert(world->universe);

    pone_val*code = va_arg(args, pone_val*);
    assert(pone_type(code) == PONE_CODE);

    pone_thread_start(world->universe, code);

    return pone_nil();
}

// TODO Add builtin 'async' method and remove this.
void pone_thread_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_thread == NULL);

    universe->class_thread = pone_class_new(world, "Thread", strlen("Thread"));
    pone_class_push_parent(world, universe->class_thread, universe->class_any);
    PONE_REG_METHOD(universe->class_thread, "start", meth_thread_start);
    pone_class_compose(world, universe->class_thread);
    pone_universe_set_global(universe, "Thread", universe->class_thread);
    assert(universe->class_thread->as.obj.klass);
}

