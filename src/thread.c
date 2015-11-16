#include "pone.h"
#include <errno.h>
#include <signal.h>

typedef struct thread_context {
    pone_world* world;
    pone_val* code;
} thread_context;

static void* thread_start(void* p) {

    thread_context* context = (thread_context*)p;

    THREAD_TRACE("NEW %lx world:%p", pthread_self(), context->world);

    // extract values to stack
    pone_world* world = context->world;
    pone_val*   code = context->code;
    assert(code);
    assert(pone_type(code) == PONE_CODE);
    assert(world->universe);

    // free the context object.
    pone_free(world->universe, p);

    assert(pone_type(code) == PONE_CODE);
    (void) pone_code_call(world, code, pone_nil(), 0);

    pone_universe* universe = world->universe;
    UNIVERSE_LOCK(universe);
    pone_world_free(world);
    CHECK_PTHREAD(pthread_cond_signal(&(universe->thread_temrinate_cond)));
    UNIVERSE_UNLOCK(universe);

    return NULL;
}

static pone_val* meth_thread_start(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);
    assert(world->universe);

    pone_val*code = va_arg(args, pone_val*);
    assert(pone_type(code) == PONE_CODE);

    pone_world* new_world = pone_world_new(world->universe);

    // save `code`
    GC_LOCK(world->universe);
    pone_save_tmp(new_world, code);
    GC_UNLOCK(world->universe);

    thread_context* p = pone_malloc(world->universe, sizeof(thread_context));
    p->world = new_world;
    p->code = code;

    int e;
    if ((e = pthread_create(&(new_world->thread_id), NULL, &thread_start, p)) != 0) {
        errno=e;
        perror("Cannot create thread");
        abort();
    }

    pone_val* thr = pone_obj_new(world, world->universe->class_thread);
    pone_obj_set_ivar(world, thr, "$!thread", pone_int_new(world, (pone_int_t)(&(world->thread_id))));

    return thr;
}

static pone_val* meth_thread_id(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);
    assert(sizeof(pthread_t) <= sizeof(pone_int_t));

    pone_val* thread = pone_obj_get_ivar(world, self, "$!thread");
    pthread_t* thr = (pthread_t*)pone_int_val(thread);

    return pone_int_new(world, *thr);
}

void pone_thread_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_thread == NULL);

    universe->class_thread = pone_class_new(world, "Thread", strlen("Thread"));
    pone_class_push_parent(world, universe->class_thread, universe->class_any);
    pone_add_method_c(world, universe->class_thread, "start", strlen("start"), meth_thread_start);
    pone_add_method_c(world, universe->class_thread, "id", strlen("id"), meth_thread_id);
    pone_class_compose(world, universe->class_thread);
    pone_universe_set_global(universe, "Thread", universe->class_thread);
    assert(universe->class_thread->as.obj.klass);
}

