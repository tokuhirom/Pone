%{

#include "pvip.h"
#include "pvip_private.h"
#include <assert.h>

#define YYSTYPE pone_node*
#define YY_NAME(n) PVIP_##n
#define YY_XTYPE PVIPParserContext
#define NOP() PVIP_node_new_children(&(G->data), PVIP_NODE_NOP)
#define MAYBE(n) (n) ? (n) : NOP()
#define PARSER (&(G->data))
#define NEWLINE G->data.line_number++
#define ENTER do { \
        G->data.line_number_stack_size++; \
        G->data.line_number_stack = realloc(G->data.line_number_stack, sizeof(int)*G->data.line_number_stack_size); \
        if (!G->data.line_number_stack) { \
            abort(); \
        } \
        G->data.line_number_stack[G->data.line_number_stack_size-1] = G->data.line_number; \
    } while (0)
#define LEAVE do { assert(G->data.line_number_stack_size> 0); G->data.line_number_stack_size--; } while (0)

#define CHILDREN(t)   PVIP_node_new_children(&(G->data),t)
#define CHILDREN1(t,a)   PVIP_node_new_children1(&(G->data),t,a)
#define CHILDREN2(t,a,b) PVIP_node_new_children2(&(G->data),t,a,b)
#define CHILDREN3(t,a,b,c) PVIP_node_new_children3(&(G->data),t,a,b,c)
#define CHILDREN4(t,a,b,c,d) PVIP_node_new_children4(&(G->data),t,a,b,c,d)
#define CHILDREN5(t,a,b,c,d,e) PVIP_node_new_children5(&(G->data),t,a,b,c,d,e)

/*

    A  Level             Examples
    =  =====             ========
    N  Terms             42 3.14 "eek" qq["foo"] $x :!verbose @$array
    L  Method postfix    .meth .+ .? .* .() .[] .{} .<> .«» .:: .= .^ .:
    N  Autoincrement     ++ --
    R  Exponentiation    **
    L  Symbolic unary    ! + - ~ ? || +^ ~^ ?^ ^
    L  Multiplicative    * / % %% & +< +> ~< ~> ?& div mod gcd lcm
    L  Additive          + - | ^
    L  Replication       x xx
    X  Concatenation     ~
    L  Named unary       temp let
    N  Structural infix  but does <=> leg cmp .. ..^ ^.. ^..^
    C  Chaining infix    != == < <= > >= eq ne lt le gt ge ~~ === eqv !eqv (<) (elem)
    X  Tight and         &&
    X  Tight or          || ^^ // min max
    R  Conditional       ? :
    R  Item assignment   = => += -= **= xx= .=
    L  Loose unary       so not
    X  Comma operator    , :
    R  List prefix       print push say die map substr ... [+] [*] any Z=
    X  Loose and         and
    X  Loose or          or
    N  Terminator        ; {...} unless extra ) ] }

*/

static int node_all_children_are(pone_node * node, PVIP_node_type_t type) {
    int i;
    for (i=0; i<node->children.size; ++i) {
        if (node->children.nodes[i]->type != type) {
            return 0;
        }
    }
    return 1;
}

static char PVIP_input(char *buf, YY_XTYPE D) {
    if (D.is_string) {
        if (D.str->len == D.str->pos) {
            return 0;
        } else {
            *buf = D.str->buf[D.str->pos];
            D.str->pos = D.str->pos+1;
            return 1;
        }
    } else {
        char c = fgetc(D.fp);
        *buf = c;
        return (c==EOF) ? 0 : 1;
    }
}

#define YY_INPUT(buf, result, max_size, D)		\
    result = PVIP_input(buf, D);

#define S_LAST \
    kv_A(G->data.literal_str_stack, kv_size(G->data.literal_str_stack)-1)

#define STR_INIT() \
    kv_push(pone_node*, G->data.literal_str_stack, PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, "", 0))

#define STR_POP() kv_pop(G->data.literal_str_stack)

/* Append string to current string literal */
#define APPEND_S(s, l) \
    S_LAST=PVIP_node_append_string(PARSER, S_LAST, (s), (l))

/* Append node to current string literal */
#define APPEND_N(e) \
    S_LAST=PVIP_node_append_string_node(PARSER, S_LAST, e)

/* Append dec number to current string literal */
#define APPEND_DEC(s,l) \
    S_LAST=PVIP_node_append_string_from_dec(PARSER, S_LAST, yytext, yyleng)

/* Append hex number to current string literal */
#define APPEND_HEX(s,l) \
    S_LAST=PVIP_node_append_string_from_hex(PARSER, S_LAST, yytext, yyleng)

/* Append oct number to current string literal */
#define APPEND_OCT(s,l) \
    S_LAST=PVIP_node_append_string_from_oct(PARSER, S_LAST, yytext, yyleng)


%}

comp_init = BOM? pod? e:statementlist - end-of-file {
    $$ = (G->data.root = e);
}
    | BOM? pod? ws* end-of-file {
      $$ = (G->data.root = PVIP_node_new_children(&(G->data), PVIP_NODE_STATEMENTS));
    }

BOM='\357' '\273' '\277'

statementlist =
    (
        s1:statement {
            $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_STATEMENTS);
            PVIP_node_push_child($$, s1);
            s1 = $$;
        }
        (
            - s2:statement {
                PVIP_node_push_child(s1, s2);
                $$=s1;
            }
        )* eat_terminator?
    )
    | ws+ { $$=PVIP_node_new_children(&(G->data), PVIP_NODE_STATEMENTS); }

# TODO
statement =
        - (
              while_stmt
            | use_stmt
            | enum_stmt
            | if_stmt
            | for_stmt
            | unless_stmt
            | module_stmt
            | has_stmt
            | '...' { $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_STUB); }
            | funcdef - ';'*
            | bl:block ';'*
            | 'END' - b:block { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_END, b) }
            | 'BEGIN' - b:block { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_BEGIN, b) }
            | b:normal_or_postfix_stmt { $$ = b; }
            | ';'+ {
                $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_NOP);
            }
          )

normal_or_postfix_stmt =
    n:normal_stmt (
          ( ' '+ 'if' - cond_if:expr - eat_terminator ) { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_IF, cond_if, n); }
        | ( ' '+ 'unless' - cond_unless:expr - eat_terminator ) { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_UNLESS, cond_unless, n); }
        | ( ' '+ 'for' - cond_for:expr - eat_terminator ) { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_FOR, cond_for, n); }
        | ( ' '+ 'while' - cond_for:expr - eat_terminator ) { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_WHILE, cond_for, n); }
        | ( - eat_terminator ) { $$=n; }
    )

enum_stmt =
    'my' ws+ 'enum' ws+ i:ident - e:expr { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_MY, PVIP_node_new_children2(&(G->data), PVIP_NODE_ENUM, i, e)); }
    | 'enum' ws+ i:ident - q:qw { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_ENUM, i, q); }

last_stmt = 'last' { $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_LAST); }

next_stmt = 'next' { $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_NEXT); }

has_stmt =
    {default=NULL; } 'has' ws+ v:attr_vars ( -  '=' - default:expr )? eat_terminator {
        $$ = CHILDREN2(PVIP_NODE_HAS, v, MAYBE(default));
    }

# $.var
# $!var
# @!var
# @.var
attr_vars =
    < [$@] [.!] [a-z]+> {
        $$=PVIP_node_new_string(&(G->data), PVIP_NODE_ATTRIBUTE_VARIABLE, yytext, yyleng);
    }
    | scalar

method_stmt =
    { p=NULL; } 'method' ws - i:ident ( - '(' - p:params? - ')' )? - b:block { $$ = PVIP_node_new_children3(&(G->data), PVIP_NODE_METHOD, i, MAYBE(p), b); }

normal_stmt = return_stmt | last_stmt | next_stmt | expr

return_stmt = 'return' ws e:expr { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_RETURN, e); }

module_stmt = 'module' ws pkg:pkg_name eat_terminator { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_MODULE, pkg); }

# use inet io/socket/inet;
# use io/socket/inet;
# use . io/socket/inet;
use_stmt =
    'use' - id:ident - pkg:pkg_name eat_terminator {
        $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_USE, id, pkg); 
    }
    | 'use' - '.' - pkg:pkg_name eat_terminator {
        $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_USE,
          PVIP_node_new_string(&(G->data), PVIP_NODE_IDENT, ".", 1), pkg
        );
    }
    | 'use' - pkg:pkg_name eat_terminator {
        $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_USE, NOP(), pkg); 
    }

pkg_name = < [a-zA-Z] [a-zA-Z0-9_]* ( '/' [a-zA-Z0-9_]+ )* > {
    $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_IDENT, yytext, yyleng);
}

while_stmt =
    w:while_until - cond:expr { PVIP_node_push_child(w,cond); } - (
            b:block {
                PVIP_node_push_child(w,b);
                $$ = w;
            }
            | l:lambda {
                PVIP_node_push_child(w,l);
                $$ = w;
            }
        )

while_until =
    'while' { $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_WHILE); }

for_stmt =
    'for' - src:expr - body:block { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_FOR, src, body); }
    | 'for' - src:expr - body:lambda { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_FOR, src, body); }

unless_stmt = 
        { body=NULL; } 'unless' - cond:expr - '{' - body:statementlist? - '}' {
            $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_UNLESS, cond, MAYBE(body));
        }
        # workaround for `unless Mu { }`
        | { body=NULL; } 'unless' - cond:ident - '{' - body:statementlist? - '}' {
            $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_UNLESS, cond, MAYBE(body));
        }

if_stmt = { if_body=NULL; } 'if' - if_cond:expr - '{' - if_body:statementlist? - '}' {
            $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_IF, if_cond, MAYBE(if_body));
            if_cond=$$;
        }
        (
            ws+ 'elsif' - elsif_cond:expr - '{' - elsif_body:statementlist - '}' {
                // elsif_body.change_type(PVIP_NODE_ELSIF);
                $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_ELSIF, elsif_cond, elsif_body);
                // if_cond.push_child(elsif_cond);
                PVIP_node_push_child(if_cond, $$);
            }
        )*
        (
            ws+ 'else' ws+ - '{' - else_body:statementlist? - '}' {
                if (else_body) {
                    $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_ELSE, else_body);
                    PVIP_node_push_child(if_cond, $$);
                }
            }
        )? { $$=if_cond; }

paren_args = '(' - a:expr - ','? - ')' {
        if (a->type == PVIP_NODE_LIST) {
            $$ = a;
            PVIP_node_change_type($$, PVIP_NODE_ARGS);
        } else {
            $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_ARGS, a);
        }
    }
    | '(' - ')' { $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_ARGS); }

bare_args = a:loose_unary_expr {
        $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_ARGS, a);
        a=$$;
    } (
        - ',' - b:loose_unary_expr {
            PVIP_node_push_child(a, b);
            $$=a;
        }
    )*

expr = loose_or_expr

loose_or_expr =
    f1:loose_and_expr (
        - 'or' ![a-zA-Z0-9_] - f2:loose_and_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_LOGICAL_OR, f1, f2); f1=$$; }
    )* { $$=f1; }

loose_and_expr =
    f1:list_prefix_expr (
        - 'and' ![a-zA-Z0-9_]      - f2:list_prefix_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_LOGICAL_AND, f1, f2); f1=$$; }
    )* { $$=f1; }

list_prefix_expr =
    '[' a:reduce_operator ']' - b:comma_operator_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_REDUCE, a, b); }
    | l:comma_operator_expr { $$=l; } (
        # infix:<=>, list assignment
        - '=' !'=' - r:list_prefix_expr {
            l = CHILDREN2(PVIP_NODE_LIST_ASSIGNMENT, l, r);
            $$=l;
        }
        # infix:<:=>, run-time binding
        | - ':=' !'=' - r:list_prefix_expr {
            l = CHILDREN2(PVIP_NODE_BIND, l, r);
            $$=l;
        }
        # infix:<::=>, bind and make readonly
        | - '::=' !'=' - r:list_prefix_expr {
            l = CHILDREN2(PVIP_NODE_BINDAND_MAKE_READONLY, l, r);
            $$=l;
        }
    )*

reduce_operator =
    < '*' > { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, yytext, yyleng); }
    | < '+' ![<>=] > { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, yytext, yyleng); }
    | < '-' > { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, yytext, yyleng); }
    | < '<=' > { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, yytext, yyleng); }
    | < '>=' > { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, yytext, yyleng); }

comma_operator_expr = a:loose_unary_expr { $$=a; } ( - ',' - b:loose_unary_expr {
        if (a->type==PVIP_NODE_LIST) {
            PVIP_node_push_child(a, b);
            $$=a;
        } else {
            $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_LIST, a, b);
            a=$$;
        }
    } )* ( - ',' {
        if (a->type==PVIP_NODE_LIST) {
            $$=a;
        } else {
            $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_LIST, a);
            a=$$;
        }
    } )?

# L
loose_unary_expr =
    'not' ![-a-zA-Z0-9] - f1:loose_unary_expr { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_NOT, f1); }
    | f1:item_assignment_expr { $$=f1 }

item_assignment_expr =
    a:conditional_expr (
        - (
              '=>'  - b:item_assignment_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_PAIR,             a, b); a=$$; }
            | '+='  - b:item_assignment_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_INPLACE_ADD,      a, b); a=$$; }
            | '-='  - b:item_assignment_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_INPLACE_SUB,      a, b); a=$$; }
            | '*='  - b:item_assignment_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_INPLACE_MUL,      a, b); a=$$; }
            | '/='  - b:item_assignment_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_INPLACE_DIV,      a, b); a=$$; }
            | '%='  - b:item_assignment_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_INPLACE_MOD,      a, b); a=$$; }
            | '**=' - b:item_assignment_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_INPLACE_POW,      a, b); a=$$; }
            | '|=' - b:item_assignment_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_INPLACE_BIN_OR,   a, b); a=$$; }
            | '&=' - b:item_assignment_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_INPLACE_BIN_AND,  a, b); a=$$; }
            | '^=' - b:item_assignment_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_INPLACE_BIN_XOR,  a, b); a=$$; }
            | '<<=' - b:item_assignment_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_INPLACE_BLSHIFT,  a, b); a=$$; }
            | '>>=' - b:item_assignment_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_INPLACE_BRSHIFT,  a, b); a=$$; }
            | '~='  - b:item_assignment_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_INPLACE_CONCAT_S, a, b); a=$$; }
        )
    )* { $$=a; }

conditional_expr = e1:tight_or - '?' - e2:tight_or - ':' - e3:tight_or { $$ = PVIP_node_new_children3(&(G->data), PVIP_NODE_CONDITIONAL, e1, e2, e3); }
                | tight_or

tight_or = f1:tight_and (
        - '||' - f2:tight_and { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_LOGICAL_OR, f1, f2); f1 = $$; }
        | - '//' - f2:tight_and { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_DOR, f1, f2); f1 = $$; }
    )* { $$ = f1; }

tight_and = f1:chaining_infix_expr (
        - '&&' - f2:chaining_infix_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_LOGICAL_AND, f1, f2); f1 = $$; }
    )* { $$ = f1; }

#  C  Chaining infix    != == < <= > >= eq ne lt le gt ge ~~ === eqv !eqv (<) (elem)
chaining_infix_expr = f1:structural_infix_expr { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_CHAIN, f1); f1=$$; } (
          - '==='  - f2:structural_infix_expr { pone_node* tmp = PVIP_node_new_children1(&(G->data), PVIP_NODE_VALUE_IDENTITY,          f2); PVIP_node_push_child(f1, tmp); }
        | - '==' !'=' - f2:structural_infix_expr { pone_node* tmp = PVIP_node_new_children1(&(G->data), PVIP_NODE_EQ,          f2); PVIP_node_push_child(f1, tmp); }
        | - '!='  - f2:structural_infix_expr { pone_node* tmp = PVIP_node_new_children1(&(G->data), PVIP_NODE_NE,          f2); PVIP_node_push_child(f1, tmp); }
        | - '<'   - f2:structural_infix_expr { pone_node* tmp = PVIP_node_new_children1(&(G->data), PVIP_NODE_LT,          f2); PVIP_node_push_child(f1, tmp); }
        | - '<='  - f2:structural_infix_expr { pone_node* tmp = PVIP_node_new_children1(&(G->data), PVIP_NODE_LE,          f2); PVIP_node_push_child(f1, tmp); }
        | - '>'   - f2:structural_infix_expr { pone_node* tmp = PVIP_node_new_children1(&(G->data), PVIP_NODE_GT,          f2); PVIP_node_push_child(f1, tmp); }
        | - '>='  - f2:structural_infix_expr { pone_node* tmp = PVIP_node_new_children1(&(G->data), PVIP_NODE_GE,          f2); PVIP_node_push_child(f1, tmp); }
        | - '~~'  - f2:structural_infix_expr { pone_node* tmp = PVIP_node_new_children1(&(G->data), PVIP_NODE_SMART_MATCH, f2); PVIP_node_push_child(f1, tmp); }
        | - '!~~' - f2:structural_infix_expr { pone_node* tmp = PVIP_node_new_children1(&(G->data), PVIP_NODE_NOT_SMART_MATCH, f2); PVIP_node_push_child(f1, tmp); }
        | - 'eqv' - f2:structural_infix_expr { pone_node* tmp = PVIP_node_new_children1(&(G->data), PVIP_NODE_EQV,         f2); PVIP_node_push_child(f1, tmp); }
        | - 'eq'  - f2:structural_infix_expr { pone_node* tmp = PVIP_node_new_children1(&(G->data), PVIP_NODE_STREQ,       f2); PVIP_node_push_child(f1, tmp); }
        | - 'ne'  - f2:structural_infix_expr { pone_node* tmp = PVIP_node_new_children1(&(G->data), PVIP_NODE_STRNE,       f2); PVIP_node_push_child(f1, tmp); }
        | - 'gt'  - f2:structural_infix_expr { pone_node* tmp = PVIP_node_new_children1(&(G->data), PVIP_NODE_STRGT,       f2); PVIP_node_push_child(f1, tmp); }
        | - 'ge'  - f2:structural_infix_expr { pone_node* tmp = PVIP_node_new_children1(&(G->data), PVIP_NODE_STRGE,       f2); PVIP_node_push_child(f1, tmp); }
        | - 'lt'  - f2:structural_infix_expr { pone_node* tmp = PVIP_node_new_children1(&(G->data), PVIP_NODE_STRLT,       f2); PVIP_node_push_child(f1, tmp); }
        | - 'le'  - f2:structural_infix_expr { pone_node* tmp = PVIP_node_new_children1(&(G->data), PVIP_NODE_STRLE,       f2); PVIP_node_push_child(f1, tmp); }
    )* { if (f1->children.size==1) { $$=f1->children.nodes[0]; } else { $$=f1; } }

structural_infix_expr =
    a1:named_unary_expr (
        - '..' !'.' - a2:named_unary_expr { $$=PVIP_node_new_children2(&(G->data), PVIP_NODE_RANGE, a1, a2); a1=$$; }
        | - 'cmp' ![a-z] - a2:named_unary_expr { $$=PVIP_node_new_children2(&(G->data), PVIP_NODE_CMP, a1, a2); a1=$$; }
        | - '<=>' - a2:named_unary_expr { $$=PVIP_node_new_children2(&(G->data), PVIP_NODE_NUM_CMP, a1, a2); a1=$$; }
    )? { $$=a1; }

funcall =
    !reserved i:ident - a:paren_args {
        $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_FUNCALL, i, a);
    }
    | !reserved i:ident ws+ !'{' a:bare_args {
        $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_FUNCALL, i, a);
    }

#  L  Named unary       temp let
named_unary_expr =
    {type=NULL; } 'my' ws+ type:ident? - a:concatenation_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_MY, MAYBE(type), a); }
    | 'our' ws+ a:concatenation_expr { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_OUR, a); }
    | !reserved i:ident ws+ a:bare_args { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_FUNCALL, i, a); }
    | concatenation_expr

#  X  Concatenation     ~
concatenation_expr =
    replication_expr

#  L  Replication       x xx
# TODO: xx
replication_expr =
    l:additive_expr (
        - (
            'x'  ![a-zA-Z0-9_] - r:additive_expr {
                $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_REPEAT_S, l, r);
                l=$$;
            }
        )
    )* { $$=l; }

additive_expr =
    l:multiplicative_expr (
        - '+' ![|<>=] - r1:multiplicative_expr {
            $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_ADD, l, r1);
            l = $$;
          }
        | - '-' !'-' - r2:multiplicative_expr {
            $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_SUB, l, r2);
            l = $$;
          }
        | - '~'  !'~' - r2:multiplicative_expr {
            $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_STRING_CONCAT, l, r2);
            l = $$;
          }
        | - '|' ![<>=] - r1:multiplicative_expr {
            $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_BITWISE_OR, l, r1);
            l = $$;
          }
        | - '^' ![<>=] - r1:multiplicative_expr {
            $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_BITWISE_XOR, l, r1);
            l = $$;
        }
    )* {
        $$ = l;
    }

multiplicative_expr =
    l:symbolic_unary (
        - '*' - r:symbolic_unary {
            $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_MUL, l, r);
            l = $$;
        }
        | - '/' !'/' - r:symbolic_unary {
            $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_DIV, l, r);
            l = $$;
        }
        | - '%' !'%' - r:symbolic_unary {
            $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_MOD, l, r);
            l = $$;
        }
        | - '>>' - r:symbolic_unary {
            $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_BRSHIFT, l, r);
            l = $$;
        }
        | - '<<' - r:symbolic_unary {
            $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_BLSHIFT, l, r);
            l = $$;
        }
        | - '&' ![<>=] - r1:multiplicative_expr {
            $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_BITWISE_AND, l, r1);
            l = $$;
        }
    )* {
        $$ = l;
    }

#  L  Symbolic unary    ! + - ~ ? | || +^ ~^ ?^ ^
symbolic_unary =
    '+' !'^' - f1:exponentiation_expr { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_UNARY_PLUS, f1); }
    | '-' !'-' - f1:exponentiation_expr { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_UNARY_MINUS, f1); }
    | '!' - f1:symbolic_unary { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_NOT, f1); }
    | '~' - f1:exponentiation_expr { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_STRINGIFY, f1); }
    | '?' - f1:exponentiation_expr { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_UNARY_BOOLEAN, f1); }
    | '^' - f1:exponentiation_expr { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_UNARY_UPTO, f1); }
    | exponentiation_expr

exponentiation_expr = 
    f1:autoincrement_expr (
        - '**' - f2:autoincrement_expr {
            $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_POW, f1, f2);
            f1=$$;
        }
    )* {
        $$=f1;
    }

# ++, -- is not supported yet
autoincrement_expr =
      '++' v:variable { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_PREINC, v); }
    | '--' v:variable { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_PREDEC, v); }
    | n:method_postfix_expr (
        '++' { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_POSTINC, n); }
        | '--' { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_POSTDEC, n); }
        | '' { $$=n; }
    )

method_postfix_expr =
          f1:term { $$=f1; } (
              '{' - k:term - '}' { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_ATKEY, f1, k); f1=$$; }
            | '<' - k:atkey_key - '>' {  $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_ATKEY, f1, k); f1=$$; }
            | '.^' f2:ident { f3 = NULL; } f3:paren_args? {
                $$ = PVIP_node_new_children3(&(G->data), PVIP_NODE_META_METHOD_CALL, f1, f2, MAYBE(f3));
                f1=$$;
            }
            | '[' - f2:expr - ']' {
                $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_ATPOS, f1, f2);
                f1=$$;
            }
            | '.' f2:ident (
                f3:paren_args {
                    $$ = PVIP_node_new_children3(&(G->data), PVIP_NODE_METHODCALL, f1, f2, f3);
                    f1=$$;
                }
            )
            | '.' f2:ident { $$=PVIP_node_new_children2(&(G->data), PVIP_NODE_METHODCALL, f1, f2); f1=$$; }
            | '.' f2:string f3:paren_args {
                /* Rakudo says "Quoted method name requires parenthesized arguments" */
                $$ = PVIP_node_new_children3(&(G->data), PVIP_NODE_METHODCALL, f1, f2, f3);
                f1=$$;
            }
            | a:paren_args { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_FUNCALL, f1, a); f1=$$; }
          )*

atkey_key = < [^>]+ > { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, yytext, yyleng); }

term = 
    complex
    | integer
    | dec_number
    | string
    | '(' - e:expr  - ')' { $$ = e; }
    | variable
    | '$?LINE' { $$ = PVIP_node_new_int(&(G->data), PVIP_NODE_INT, G->data.line_number); }
    | array
    | class
    | role
    | funcall
    | qw
    | hash
    | lambda
    | it_method
    | enum
    | 'nil' ![-a-zA-Z0-9] { $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_NIL); }
    | 'try' ws - b:block { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_TRY, b); }
    | 'try' ws+ b:expr { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_TRY, b); }
    | regexp
    | 'True' ![-a-zA-Z0-9] { $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_TRUE); }
    | 'False' ![-a-zA-Z0-9] { $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_FALSE); }
    | !reserved ident
    | < 'class' > { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_IDENT, yytext, yyleng); }
    | pair
    | '\\' t:term { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_REF, t); }
    | '(' - ')' { $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_LIST); }
    | funcref
    | '*' ![*=] { $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_WHATEVER); }
    | attr_vars

enum =
    'enum' ws+ q:qw { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_ENUM, PVIP_node_new_children(&(G->data), PVIP_NODE_NOP), q); }

funcref = '&' i:ident { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_FUNCREF, i); }

twvars = 
    < '$*' [A-Z] [A-Z_]+ > { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_TWVAR, yytext, yyleng); }
    | '$^a' { $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_TW_A); }
    | '$^b' { $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_TW_B); }
    | '$^c' { $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_TW_C); }

reserved = ( 'True' | 'False' | 'my' | 'our' | 'while' | 'unless' | 'if' | 'elsif' | 'else' | 'role' | 'class' | 'try' | 'has' | 'sub' | 'cmp' | 'enum' | 'END' | 'BEGIN' | 'not' | 'and' | 'or' | 'use' | 'nil' ) ![-A-Za-z0-9]

role =
    'role' ws+ i:ident - b:block { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_ROLE, i, b); }

# TODO optimizable
class =
    class_declare

class_declare =
    { i=NULL; is=NULL; } 'class'  (
        ws+ i:ident
    )? - is:is_does_list? - b:block {
        $$ = PVIP_node_new_children3(&(G->data), 
            PVIP_NODE_CLASS,
            MAYBE(i),
            MAYBE(is),
            b
        );
    }

# XXX Bad Code
is_does_list =
    'is' ws+ a:ident { a=PVIP_node_new_children1(&(G->data), PVIP_NODE_LIST, PVIP_node_new_children1(&(G->data), PVIP_NODE_IS, a)); } (
        - 'is' ws+ b:ident { PVIP_node_push_child(a, PVIP_node_new_children1(&(G->data), PVIP_NODE_IS, b)); }
        | - 'does' ws+ b:ident { PVIP_node_push_child(a, PVIP_node_new_children1(&(G->data), PVIP_NODE_DOES, b)); }
    )* { $$=a; }
    | 'does' ws+ a:ident { a=PVIP_node_new_children1(&(G->data), PVIP_NODE_LIST, PVIP_node_new_children1(&(G->data), PVIP_NODE_DOES, a)); } (
        - 'is' ws+ b:ident { PVIP_node_push_child(a, PVIP_node_new_children1(&(G->data), PVIP_NODE_IS, b)); }
        | - 'does' ws+ b:ident { PVIP_node_push_child(a, PVIP_node_new_children1(&(G->data), PVIP_NODE_DOES, b)); }
    )* { $$=a; }

it_method = (
        '.' i:ident { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_IT_METHODCALL, i); i=$$; }
        (
            a:paren_args { PVIP_node_push_child(i, a); }
        )?
    ) { $$=i; }

ident =
    < '::'? [A-Za-z] [-A-Za-z0-9_]* ( '::' [A-Za-z] [A-Za-z0-9_]* )* > { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_IDENT, yytext, yyleng); }
    | < [a-zA-Z_] [a-zA-Z0-9]* ( [-_] [a-zA-Z0-9]+ )* > {
        $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_IDENT, yytext, yyleng);
    }


hash = '{' -
    p1:pair { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_HASH, p1); p1=$$; } ( -  ',' - p2:pair { PVIP_node_push_child(p1, p2); $$=p1; } )*
    ','?
    - '}' { $$=p1; }
    | '{' - '}' { $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_HASH); }

pair =
    k:hash_key - '=>' - v:loose_unary_expr { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_PAIR, k, v); }
    | ':' < key:ident > '<' value:ident '>' {
        PVIP_node_change_type(key, PVIP_NODE_STRING);
        PVIP_node_change_type(value, PVIP_NODE_STRING);
        $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_PAIR, key, value);
    }
    | ':' < key:ident > '(' value:expr ')' {
        PVIP_node_change_type(key, PVIP_NODE_STRING);
        $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_PAIR, key, value);
    }
    | ':' < key:ident > '[' - value:expr - ']' {
        PVIP_node_change_type(key, PVIP_NODE_STRING);
        $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_PAIR, key, value);
    }
    | ':' < [a-z]+ > { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_PAIR, PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, yytext, yyleng), PVIP_node_new_children(&(G->data), PVIP_NODE_TRUE)); }
    | ':!' < [a-z]+ > { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_PAIR, PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, yytext, yyleng), PVIP_node_new_children(&(G->data), PVIP_NODE_FALSE)); }
    | ':' v:variable {
        $$ = PVIP_node_new_children2(&(G->data), 
            PVIP_NODE_PAIR,
            PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, v->pv->buf, v->pv->len),
            v
        );
    }

hash_key =
    < [a-zA-Z0-9_]+ > { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, yytext, yyleng); }
    | string

qw =
    '<<' - qw_list - '>>'
    | '<' - qw_list - '>'
    | '<' - '>' { $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_LIST); }

qw_list =
        a:qw_item { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_LIST, a); a = $$; }
        ( ws+ b:qw_item { PVIP_node_push_child(a, b); $$ = a; } )*
        { $$=a; }

# I want to use [^ ] but greg does not support it...
# https://github.com/nddrylliog/greg/issues/12
qw_item = < [^ >\n]+ > { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, yytext, yyleng); }

# TODO optimize
funcdef =
    'my' ws - f:funcdef { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_MY, NOP(), f); }
    | 'our' ws - f:funcdef { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_OUR, f); }
    | 'sub' { p=NULL; } ws+ i:ident - '(' - p:params? - ')' - b:block {
        if (!p) {
            p = PVIP_node_new_children(&(G->data), PVIP_NODE_PARAMS);
        }
        /* remove NOP(). it's backward compatible feature. */
        $$ = PVIP_node_new_children4(&(G->data), PVIP_NODE_FUNC, i, p, NOP(), b);
    }
    | 'sub' ws+ i:ident - b:block {
        pone_node* pp = PVIP_node_new_children(&(G->data), PVIP_NODE_PARAMS);
        $$ = PVIP_node_new_children4(&(G->data), PVIP_NODE_FUNC, i, pp, NOP(), b);
    }

lambda =
    {p=NULL; } '->' - ( !'{' p:params )? - b:block {
        if (!p) {
            p = PVIP_node_new_children(&(G->data), PVIP_NODE_PARAMS);
        }
        $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_LAMBDA, p, b);
    }
    | b:block { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_LAMBDA, b); }
    | {p=NULL; } 'sub' ![-a-zA-Z0-9] - ( !'{' p:params)? - b:block {
        if (!p) {
            p = PVIP_node_new_children(&(G->data), PVIP_NODE_PARAMS);
        }
        $$ = PVIP_node_new_children4(&(G->data), PVIP_NODE_FUNC, PVIP_node_new_children(&(G->data), PVIP_NODE_NOP), p, NOP(), b);
    }

params =
    v:param { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_PARAMS, v); v=$$; }
    ( - ',' - v1:param { PVIP_node_push_child(v, v1); $$=v; } )*
    { $$=v; }

# Str $x=""
param =
    { i=NULL; d=NULL; }
    (
        ( i:ident ws+ )? # type
        v:param_term
        (
            - (
                d:param_defaults
            )
        )*
    ) {
        int attr = 0;
        $$ = CHILDREN4(PVIP_NODE_PARAM, MAYBE(i), v, MAYBE(d), PVIP_node_new_int(&(G->data), PVIP_NODE_INT, attr));
    }

param_term =
    term

param_defaults =
    '=' - e:expr { $$=e; }

block =
    { ENTER; s=NULL; } ('{' - s:statementlist? - '}' !'.') {
        if (!s) {
            /* empty block */
            $$=PVIP_node_new_children(&(G->data), PVIP_NODE_BLOCK);
        } else if (s->children.nodes[0]->type == PVIP_NODE_PAIR) {
            PVIP_node_change_type(s, PVIP_NODE_HASH);
            $$=s;
        } else if (s->children.nodes[0]->type == PVIP_NODE_LIST && node_all_children_are(s->children.nodes[0], PVIP_NODE_PAIR)) {
            PVIP_node_change_type(s, PVIP_NODE_HASH);
            $$=s;
        } else {
            $$=PVIP_node_new_children1(&(G->data), PVIP_NODE_BLOCK, s);
        }
        LEAVE;
    }

# XXX optimizable
array =
    '[' - e:expr - ']' {
        if (PVIP_node_category(e->type) == PVIP_CATEGORY_CHILDREN && e->type == PVIP_NODE_LIST) {
            PVIP_node_change_type(e, PVIP_NODE_ARRAY);
            $$=e;
        } else {
            $$=PVIP_node_new_children1(&(G->data), PVIP_NODE_ARRAY, e);
        }
    }
    | '[' - ']' { $$ = PVIP_node_new_children(&(G->data), PVIP_NODE_ARRAY); }

my = 
    { type=NULL; } 'my' ws+ type:ident? - v:variable { $$ = PVIP_node_new_children2(&(G->data), PVIP_NODE_MY, MAYBE(type), v); }
    | 'my' ws+ '(' - v:bare_variables - ')' { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_MY, v); }
    | 'our' ws+ v:variable { $$ = PVIP_node_new_children1(&(G->data), PVIP_NODE_OUR, v); }

bare_variables =
    v1:variable { v1=PVIP_node_new_children1(&(G->data), PVIP_NODE_LIST, v1); } (
        - ',' - v2:variable { PVIP_node_push_child(v1, v2); }
    )* { $$=v1; }

variable = scalar | twvars | funcref | attr_vars

scalar =
    < '$' varname > { assert(yyleng > 0); $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_VARIABLE, yytext, yyleng); }
    | '$!' ![a-zA-Z0-9_] { $$=PVIP_node_new_children(&(G->data), PVIP_NODE_SPECIAL_VARIABLE_ERRNO); }
    | '$$' ![a-zA-Z0-9_] { $$=PVIP_node_new_children(&(G->data), PVIP_NODE_SPECIAL_VARIABLE_PID); }
    | '$@' ![a-zA-Z0-9_] { $$=PVIP_node_new_children(&(G->data), PVIP_NODE_SPECIAL_VARIABLE_EXCEPTIONS); }
    | '$/' { $$=PVIP_node_new_children(&(G->data), PVIP_NODE_SPECIAL_VARIABLE_REGEXP_MATCH); }

varname = [a-zA-Z_] ( [a-zA-Z0-9_]+ | '-' [a-zA-Z_] [a-zA-Z0-9_]* )*

#  <?MARKED('endstmt')>
#  <?terminator>
eat_terminator =
    (';' -) | end-of-file

dec_number =
    <[0-9]+ ( '.' [0-9]+ )? 'e' '-'? [0-9]+> {
    $$ = PVIP_node_new_number(&(G->data), PVIP_NODE_NUMBER, yytext, yyleng);
}
    | <([.][0-9]+)> {
    $$ = PVIP_node_new_number(&(G->data), PVIP_NODE_NUMBER, yytext, yyleng);
}
    | <([0-9]+ '.' [0-9]+)> {
    $$ = PVIP_node_new_number(&(G->data), PVIP_NODE_NUMBER, yytext, yyleng);
}
    | <([0-9_]+)> {
    $$ = PVIP_node_new_intf(&(G->data), PVIP_NODE_INT, yytext, yyleng, 10);
}

complex =
    < (
        [0-9]+ ('.' [0-9]+)?
        | ('.' [0-9]+)
    ) > 'i' { $$ = PVIP_node_new_number(&(G->data), PVIP_NODE_COMPLEX, yytext, yyleng); }

integer =
    '0b' <[01_]+> {
    $$ = PVIP_node_new_intf(&(G->data), PVIP_NODE_INT, yytext, yyleng, 2);
}
    | '0d' <[0-9]+> {
    $$ = PVIP_node_new_intf(&(G->data), PVIP_NODE_INT, yytext, yyleng, 10);
}
    | '0x' <[0-9a-fA-F_]+> {
    $$ = PVIP_node_new_intf(&(G->data), PVIP_NODE_INT, yytext, yyleng, 16);
}
    | '0o' <[0-7]+> {
    $$ = PVIP_node_new_intf(&(G->data), PVIP_NODE_INT, yytext, yyleng, 8);
}
    | ':10<' <[0-9]+> '>' {
        $$ = PVIP_node_new_intf(&(G->data), PVIP_NODE_INT, yytext, yyleng, 10);
    }
    | ':' i:integer_int '<' <[0-9a-fA-F]+> '>' {
        int base = i->iv;
        $$ = PVIP_node_new_intf(&(G->data), PVIP_NODE_INT, yytext, yyleng, base);
    }

integer_int =
    <[0-9]+> { $$ = PVIP_node_new_intf(&(G->data), PVIP_NODE_INT, yytext, yyleng, 10); }

string = dq_string | sq_string

dq_string =
    (
        '"' {
            STR_INIT();
        } (
            dq_string_inner
            | '>' { APPEND_S(">", 1); }
        )* '"' { $$=STR_POP(); }
    )
    | (
        'qq<' {
            STR_INIT();
        } (
            dq_string_inner
            | '"' { APPEND_S("\"", 1); }
        )* '>' { $$=STR_POP(); }
    )

dq_string_inner =
    (
        "\n" { G->data.line_number++; APPEND_S("\n", 1); }
        | '#{' - e:expr - "}" { APPEND_N(e); }
        | '#{' - "}" { APPEND_S("", 0); }
        | < [^>"#\\\n$%]+ > { APPEND_S(yytext, yyleng); }
        | < '#' ![{] > { APPEND_S(yytext, yyleng); }
        | h:variable '<' - k:atkey_key - '>' { APPEND_N((CHILDREN2(PVIP_NODE_ATKEY, h, k))); }
        # %hash{do_a}
        | h:variable '{' - k:expr - '}' {  APPEND_N(CHILDREN2(PVIP_NODE_ATKEY, h, k)); }
        | h:variable ( '{' - '}' | '<' - '>' ) {  APPEND_N(CHILDREN1(PVIP_NODE_STRINGIFY, h)); }
        | '%' { APPEND_S("%", 1); }
        | v:variable { APPEND_N(v); }
        | esc 'a' { APPEND_S("\a", 1); }
        | esc 'b' { APPEND_S("\b", 1); }
        | esc 't' { APPEND_S("\t", 1); }
        | esc 'r' { APPEND_S("\r", 1); }
        | esc 'n' { APPEND_S("\n", 1); }
        | esc '"' { APPEND_S("\"", 1); }
        | esc '$' { APPEND_S("\"", 1); }
        | esc '0' { APPEND_S("\0", 1); }
        | esc '{' { APPEND_S("{", 1); /* } */ }
        | esc 'c[' < [^\]]+ > ']' { APPEND_N(PVIP_node_new_string(&(G->data), PVIP_NODE_UNICODE_CHAR, yytext, yyleng)); }
        # \c10
        | esc 'c' < [0-9] [0-9] > {
            APPEND_DEC(yytext, yyleng);
        }
        | ( esc 'x' (
                    '0'? < ( [a-fA-F0-9] [a-fA-F0-9] ) >
            | '[' '0'? < ( [a-fA-F0-9] [a-fA-F0-9] ) > ']' )
        ) {
            APPEND_HEX(yytext, yyleng);
        }
        | esc 'o' < ( [0-7] [0-7] | '0' [0-7] [0-7] ) > {
            APPEND_OCT(yytext, yyleng);
        }
        | esc 'o['
                '0'? < [0-7] [0-7] > {
                    APPEND_OCT(yytext, yyleng);
                } (
            ',' '0'? < [0-7] [0-7] > {
                APPEND_OCT(yytext, yyleng);
            }
        )* ']'
        | esc esc { APPEND_S("\\", 1) }
    )

regexp_start = ( 'm/' | '/' ) { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_REGEXP, "", 0); }

regexp =
    r:regexp_start (
        <[^/]+> { r=PVIP_node_append_string(&(G->data), r, yytext, yyleng); }
       | esc '/' { r=PVIP_node_append_string(&(G->data), r, "/", 1); }
    )+ '/' { $$=r; }


esc = '\\'

sq_string = "'" { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, "", 0); } (
        "\n" { G->data.line_number++; $$=PVIP_node_append_string(&(G->data), $$, "\n", 1); }
        | < [^'\\\n]+ > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
        | esc "'" { $$=PVIP_node_append_string(&(G->data), $$, "'", 1); }
        | esc esc { $$=PVIP_node_append_string(&(G->data), $$, "\\", 1); }
        | < esc . > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
    )* "'"
    | 'q/' { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, "", 0); } (
        "\n" { G->data.line_number++; $$=PVIP_node_append_string(&(G->data), $$, "\n", 1); }
        | < [^/\\\n]+ > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
        | esc "'" { $$=PVIP_node_append_string(&(G->data), $$, "'", 1); }
        | esc "/" { $$=PVIP_node_append_string(&(G->data), $$, "/", 1); }
        | esc esc { $$=PVIP_node_append_string(&(G->data), $$, "\\", 1); }
        | < esc . > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
    )* '/'
    | 'q!' { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, "", 0); } (
        "\n" { G->data.line_number++; $$=PVIP_node_append_string(&(G->data), $$, "\n", 1); }
        | < [^!\\\n]+ > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
        | esc "'" { $$=PVIP_node_append_string(&(G->data), $$, "'", 1); }
        | esc "/" { $$=PVIP_node_append_string(&(G->data), $$, "/", 1); }
        | esc esc { $$=PVIP_node_append_string(&(G->data), $$, "\\", 1); }
        | < esc . > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
    )* '!'
    | 'q|' { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, "", 0); } (
        "\n" { G->data.line_number++; $$=PVIP_node_append_string(&(G->data), $$, "\n", 1); }
        | < [^|\\\n]+ > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
        | esc "'" { $$=PVIP_node_append_string(&(G->data), $$, "'", 1); }
        | esc "/" { $$=PVIP_node_append_string(&(G->data), $$, "/", 1); }
        | esc esc { $$=PVIP_node_append_string(&(G->data), $$, "\\", 1); }
        | < esc . > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
    )* '|'
    | 'q{' { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, "", 0); } (
        "\n" { G->data.line_number++; $$=PVIP_node_append_string(&(G->data), $$, "\n", 1); }
        | < [^}\\\n]+ > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
        | esc "'" { $$=PVIP_node_append_string(&(G->data), $$, "'", 1); }
        | esc "/" { $$=PVIP_node_append_string(&(G->data), $$, "/", 1); }
        | esc esc { $$=PVIP_node_append_string(&(G->data), $$, "\\", 1); }
        | < esc . > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
    )* '}'
    | 'q<' { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, "", 0); } (
        "\n" { G->data.line_number++; $$=PVIP_node_append_string(&(G->data), $$, "\n", 1); }
        | < [^>\\\n]+ > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
        | esc "'" { $$=PVIP_node_append_string(&(G->data), $$, "'", 1); }
        | esc ">" { $$=PVIP_node_append_string(&(G->data), $$, ">", 1); }
        | esc esc { $$=PVIP_node_append_string(&(G->data), $$, "\\", 1); }
        | < esc . > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
    )* '>'
    | 'q@' { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, "", 0); } (
        "\n" { G->data.line_number++; $$=PVIP_node_append_string(&(G->data), $$, "\n", 1); }
        | < [^@\\\n]+ > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
        | esc "'" { $$=PVIP_node_append_string(&(G->data), $$, "'", 1); }
        | esc "@" { $$=PVIP_node_append_string(&(G->data), $$, "~", 1); }
        | esc esc { $$=PVIP_node_append_string(&(G->data), $$, "\\", 1); }
        | < esc . > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
    )* '@'
    | 'q~' { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, "", 0); } (
        "\n" { G->data.line_number++; $$=PVIP_node_append_string(&(G->data), $$, "\n", 1); }
        | < [^~\\\n]+ > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
        | esc "'" { $$=PVIP_node_append_string(&(G->data), $$, "'", 1); }
        | esc "~" { $$=PVIP_node_append_string(&(G->data), $$, "~", 1); }
        | esc esc { $$=PVIP_node_append_string(&(G->data), $$, "\\", 1); }
        | < esc . > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
    )* '~'
    | 'q,' { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, "", 0); } (
        "\n" { G->data.line_number++; $$=PVIP_node_append_string(&(G->data), $$, "\n", 1); }
        | < [^,\\\n]+ > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
        | esc "," { $$=PVIP_node_append_string(&(G->data), $$, "'", 1); }
        | esc "~" { $$=PVIP_node_append_string(&(G->data), $$, "~", 1); }
        | esc esc { $$=PVIP_node_append_string(&(G->data), $$, "\\", 1); }
        | < esc . > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
    )* ','
    | 'q[' { $$ = PVIP_node_new_string(&(G->data), PVIP_NODE_STRING, "", 0); } (
        "\n" { G->data.line_number++; $$=PVIP_node_append_string(&(G->data), $$, "\n", 1); }
        | < [^\]\\\n]+ > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
        | esc "'" { $$=PVIP_node_append_string(&(G->data), $$, "'", 1); }
        | esc "]" { $$=PVIP_node_append_string(&(G->data), $$, "]", 1); }
        | esc esc { $$=PVIP_node_append_string(&(G->data), $$, "\\", 1); }
        | < esc . > { $$=PVIP_node_append_string(&(G->data), $$, yytext, yyleng); }
    )* ']'

comment =
    '#`[' [^\]]* ']'
    | '#`(' [^)]* ')'
    | '#`（' [^）]* '）'
    | '#`『' [^』]* '』'
    | '#`《' [^》]* '》'
    | '#' [^\n]* end-of-line

# white space
ws = 
    '\n' pod
    | '\n=begin END\n' .* | ' ' | '\f' | '\v' | '\t' | '\205' | '\240'
    | '\n=END\n' .*
    | end-of-line
    | comment

pod =
  (
      '=begin ' [a-z]+ ' '* '\n' { NEWLINE; }
      ( !'=end ' [^\n]* '\n' { NEWLINE; } )*
      '=end ' [a-z]+ ' '* '\n' { NEWLINE; }
  )

- = ws*

end-of-line = ( '\r\n' | '\n' | '\r' ) {
    G->data.line_number++;
}
end-of-file = !'\0'

%%

pone_node * PVIP_parse_string(pvip_t* pvip, const char *string, int len, int debug, PVIPString **error) {
    pone_node *root = NULL;

    GREG g;
    YY_NAME(init)(&g);

#ifdef YY_DEBUG
    g.debug=debug;
#endif

    g.data.line_number = 1;
    g.data.line_number_stack_size = 0;
    g.data.line_number_stack = NULL;
    g.data.is_string   = 1;
    g.data.str = malloc(sizeof(PVIPParserStringState));
    g.data.str->buf     = string;
    g.data.str->len     = len;
    g.data.str->pos     = 0;
    g.data.pvip = pvip;

    if (!YY_NAME(parse)(&g)) {
      if (error) {
        *error = PVIP_string_new();
        PVIP_string_concat(*error, "** Syntax error at line ", strlen("** Syntax error at line "));
        PVIP_string_concat_int(*error, g.data.line_number);
        PVIP_string_concat(*error, "\n", 1);
        if (g.text[0]) {
            PVIP_string_concat(*error, "** near ", strlen("** near "));
            PVIP_string_concat(*error, g.text, strlen(g.text));
        }
        if (g.pos < g.limit || g.data.str->len==g.data.str->pos) {
            g.buf[g.limit]= '\0';
            PVIP_string_concat(*error, " before text \"", strlen(" before text \""));
            while (g.pos < g.limit) {
                if ('\n' == g.buf[g.pos] || '\r' == g.buf[g.pos]) break;
                PVIP_string_concat_char(*error, g.buf[g.pos++]);
            }
            if (g.pos == g.limit) {
                while (g.data.str->len!=g.data.str->pos) {
                    char ch = g.data.str->buf[g.data.str->pos++];
                    if (!ch || '\n' == ch || '\r' == ch) {
                        break;
                    }
                    PVIP_string_concat_char(*error, ch);
                }
            }
            PVIP_string_concat_char(*error, '\"');
        }
        PVIP_string_concat(*error, "\n\n", 2);
        free(g.data.str);
      }
      goto finished;
    }
    if (g.limit!=g.pos) {
        if (error) {
            *error = PVIP_string_new();
            PVIP_string_concat(*error, "Syntax error! Around:\n", strlen("Syntax error! Around:\n"));
            int i;
            for (i=0; g.limit!=g.pos && i<24; i++) {
                char ch = g.data.str->buf[g.pos++];
                if (ch) {
                    PVIP_string_concat_char(*error, ch);
                }
            }
            PVIP_string_concat_char(*error, '\n');
        }
        goto finished;
    }
    root = g.data.root;

finished:

    free(g.data.line_number_stack);
    free(g.data.str);
    assert(g.data.root);
    YY_NAME(deinit)(&g);
    return root;
}

/*
XXX Output error message to stderr is ugly.
XXX We need to add APIs for getting error message.
 */
pone_node * PVIP_parse_fp(pvip_t* pvip, FILE *fp, int debug, PVIPString **error) {
    GREG g;
    YY_NAME(init)(&g);

#ifdef YY_DEBUG
    g.debug=debug;
#endif

    g.data.line_number = 1;
    g.data.line_number_stack_size = 0;
    g.data.line_number_stack = NULL;
    g.data.is_string   = 0;
    g.data.fp = fp;
    g.data.pvip = pvip;

    if (!YY_NAME(parse)(&g)) {
      if (error) {
        *error = PVIP_string_new();
        PVIP_string_concat(*error, "** Syntax error at line ", strlen("** Syntax error at line "));
        PVIP_string_concat_int(*error, g.data.line_number);
        PVIP_string_concat(*error, "\n", 1);
        if (g.text[0]) {
          PVIP_string_concat(*error, "** near ", strlen("** near "));
          PVIP_string_concat(*error, g.text, strlen(g.text));
        }
        if (g.pos < g.limit || !feof(fp)) {
          g.buf[g.limit]= '\0';
          PVIP_string_concat(*error, " before text \"", strlen(" before text \""));
          while (g.pos < g.limit) {
            if ('\n' == g.buf[g.pos] || '\r' == g.buf[g.pos]) break;
            PVIP_string_concat_char(*error, g.buf[g.pos++]);
          }
          if (g.pos == g.limit) {
            int c;
            while (EOF != (c= fgetc(fp)) && '\n' != c && '\r' != c)
            PVIP_string_concat_char(*error, c);
          }
          PVIP_string_concat_char(*error, '\"');
        }
        PVIP_string_concat(*error, "\n\n", 2);
      }
      return NULL;
    }
    if (!feof(fp)) {
      if (error) {
        *error = PVIP_string_new();
        PVIP_string_concat(*error, "Syntax error! At line ", strlen("Syntax error! At line "));
        PVIP_string_concat_int(*error, g.data.line_number);
        PVIP_string_concat(*error, ":\n", strlen(":\n"));
        int i;
        for (i=0; !feof(fp) && i<24; i++) {
          char ch = fgetc(fp);
          if (ch != EOF) {
            PVIP_string_concat_char(*error, ch);
          }
        }
        PVIP_string_concat_char(*error, '\n');
      }
      return NULL;
    }
    free(g.data.line_number_stack);
    free(g.data.str);
    pone_node *root = g.data.root;
    assert(g.data.root);
    YY_NAME(deinit)(&g);
    return root;
}

