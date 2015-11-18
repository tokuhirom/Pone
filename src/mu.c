#include "pone.h" /* PONE_INC */

/*

=head1 NAME

Mu - 無

=head1 DESCRIPTION

The root of the Perl 6 type hierarchy. For the origin of the name, see http://en.wikipedia.org/wiki/Mu_%28negative%29. One can also say that there are many undefined values in Perl 6, and Mu is the most undefined value.

Note that most classes do not derive from Mu directly, but rather from Any.

=head1 METHODS

=cut

*/

/*

=head3 C<Mu#say()>

    [1,2,3].say();

Prints value to $*OUT after stringification using .gist method with newline at end.

=cut

*/
static pone_val* meth_mu_say(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n==0);

    pone_val* s = pone_stringify(world, self);
    fwrite(pone_str_ptr(s), sizeof(char), pone_str_len(s), stdout);
    return pone_nil();
}

/*

=head3 C<Mu#Str()>

Returns a string representation of the invocant, intended to be machine readable. Method Str warns on type objects, and produces the empty string.

=cut

*/
static pone_val* meth_mu_str(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n==0);
    return pone_str_new_const(world, "Nil", strlen("Nil"));
}

pone_val* pone_init_mu(pone_world* world) {
    pone_val* obj = pone_obj_alloc(world, PONE_OBJ);
    obj->as.obj.ivar = kh_init(str);
    obj->as.obj.klass = pone_nil();

    pone_obj_set_ivar(world, (pone_val*)obj, "$!name", pone_str_new_const(world, "Mu", strlen("Mu")));
    pone_obj_set_ivar(world, (pone_val*)obj, "$!methods", pone_hash_new(world));
    pone_obj_set_ivar(world, (pone_val*)obj, "@!parents", pone_ary_new(world, 0));

    pone_add_method_c(world, obj, "Str", strlen("Str"), meth_mu_str);
    pone_add_method_c(world, obj, "say", strlen("say"), meth_mu_say);

    return obj;
}

