#include "pone.h"
#include <errno.h>
#include <signal.h>

typedef struct thread_context {
    pone_universe* universe;
    pone_val* code;
} thread_context;

static void* thread_start(void* p) {
    thread_context* context = (thread_context*)p;

    // extract values to stack
    pone_universe* universe = context->universe;
    pone_val* code = context->code;
    assert(code);

    // free the context object.
    pone_free(context->universe, p);

    GVL_LOCK(universe);

    pone_world* world = pone_world_new(universe);

    pone_val* retval = pone_code_call(world, code, pone_nil(), 0);

    GVL_UNLOCK(universe);

    pone_world_free(world);

    // get thread mutex
    return retval;
}

static pone_val* meth_thread_start(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);

    pone_val*code = va_arg(args, pone_val*);
    assert(pone_type(code) == PONE_CODE);

    thread_context* p = pone_malloc(world->universe, sizeof(thread_context));
    p->universe = world->universe;
    p->code = code;

    if (world->universe->thread_num == INT_MAX) {
        fprintf(stderr, "too many threads\n");
        abort();
    }
    world->universe->thread_num++;
    pone_thread_t* pthr = pone_malloc(world->universe, sizeof(pone_thread_t));
    pthr->next = world->universe->threads;
    world->universe->threads = pthr;

    int e;
    if ((e = pthread_create(&(pthr->thread), NULL, &thread_start, p)) != 0) {
        errno=e;
        perror("Cannot create thread");
        abort();
    }

    pone_val* thr = pone_obj_new(world->universe, world->universe->class_thread);
    pone_obj_set_ivar(world->universe, thr, "$!thread", pone_int_new(world->universe, (pone_int_t)(&(pthr->thread))));

    return thr;
}

static pone_val* meth_thread_id(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);
    assert(sizeof(pthread_t) <= sizeof(pone_int_t));

    pone_val* thread = pone_obj_get_ivar(world->universe, self, "$!thread");
    pthread_t* thr = (pthread_t*)pone_int_val(thread);

    return pone_int_new(world->universe, *thr);
}

pone_val* pone_thread_join(pone_universe* universe, pthread_t thr) {
    THREAD_TRACE("JOIN %x\n", thr);

    GVL_UNLOCK(universe);

    void* retval;
    int e;
    if ((e = pthread_join(thr, &retval)) != 0) {
        errno = e;
        perror("cannot join thread");
        exit(EXIT_FAILURE);
    }

    GVL_LOCK(universe);

    pone_thread_t *pthr = universe->threads;
    if (pthr->thread == thr) {
        universe->threads = pthr->next;
        pone_free(universe, pthr);
    } else {
        pone_thread_t *prev = pthr;
        pthr = pthr->next;
        while (pthr != NULL) {
            if (pthr->thread == thr) {
                prev->next = pthr->next;
                pone_free(universe, pthr);
                break;
            }
            prev = pthr;
            pthr = pthr->next;
        }
    }
    universe->thread_num--;

    return (pone_val*)retval;
}

static pone_val* meth_thread_join(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);
    assert(sizeof(pthread_t) <= sizeof(pone_int_t));

    pone_val* thread = pone_obj_get_ivar(world->universe, self, "$!thread");
    pthread_t* thr = (pthread_t*)pone_int_val(thread);

    return pone_thread_join(world->universe, *thr);
}

static pone_val* meth_thread_kill(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);
    assert(sizeof(pthread_t) <= sizeof(pone_int_t));

    pone_val* sig = va_arg(args, pone_val*);

    pone_val* thread = pone_obj_get_ivar(world->universe, self, "$!thread");
    pthread_t* thr = (pthread_t*)pone_int_val(thread);

    GVL_UNLOCK(world->universe);

    int e;
    if ((e = pthread_kill(*thr, pone_intify(world, sig))) != 0) {
        errno = e;
        perror("cannot join thread");
        exit(EXIT_FAILURE);
    }

    GVL_LOCK(world->universe);

    return pone_nil();
}

void pone_thread_init(pone_universe* universe) {
    assert(universe->class_thread == NULL);

    universe->class_thread = pone_class_new(universe, "Thread", strlen("Thread"));
    pone_class_push_parent(universe, universe->class_thread, universe->class_any);
    pone_add_method_c(universe, universe->class_thread, "start", strlen("start"), meth_thread_start);
    pone_add_method_c(universe, universe->class_thread, "id", strlen("id"), meth_thread_id);
    pone_add_method_c(universe, universe->class_thread, "join", strlen("join"), meth_thread_join);
    pone_add_method_c(universe, universe->class_thread, "kill", strlen("kill"), meth_thread_kill);
    pone_class_compose(universe, universe->class_thread);
    pone_universe_set_global(universe, "Thread", universe->class_thread);
    assert(universe->class_thread->as.obj.klass);
}

