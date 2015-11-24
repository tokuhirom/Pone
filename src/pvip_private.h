#ifndef PVIP_PRIVATE_H_
#define PVIP_PRIVATE_H_

/*
 * This file contains private functins in PVIP.
 */

typedef struct {
    const char  *buf;
    size_t len;
    size_t pos;
} PVIPParserStringState;

typedef struct {
    int line_number;
    int *line_number_stack;
    size_t line_number_stack_size;
    pone_node *root;
    int is_string; /* Parsing from string or file pointer. */
    PVIPParserStringState *str;
    pone_node *literal_str; /* temporary space for string literal */
    FILE *fp;
    pvip_t* pvip;
} PVIPParserContext;

/* node */
pone_node* PVIP_node_new_children( PVIPParserContext*parser, PVIP_node_type_t type);
pone_node* PVIP_node_new_children1(PVIPParserContext*parser, PVIP_node_type_t type, pone_node* n1);
pone_node* PVIP_node_new_children2(PVIPParserContext*parser, PVIP_node_type_t type, pone_node* n1, pone_node *n2);
pone_node* PVIP_node_new_children3(PVIPParserContext*parser, PVIP_node_type_t type, pone_node* n1, pone_node *n2, pone_node *n3);
pone_node* PVIP_node_new_children4(PVIPParserContext*parser, PVIP_node_type_t type, pone_node* n1, pone_node *n2, pone_node *n3, pone_node *n4);
pone_node* PVIP_node_new_children5(PVIPParserContext*parser, PVIP_node_type_t type, pone_node* n1, pone_node *n2, pone_node *n3, pone_node *n4, pone_node *n5);
pone_node* PVIP_node_new_int(PVIPParserContext* parser, PVIP_node_type_t type, int64_t n);
pone_node* PVIP_node_new_intf(PVIPParserContext* parser, PVIP_node_type_t type, const char *str, size_t len, int base);
pone_node* PVIP_node_new_string(PVIPParserContext* parser, PVIP_node_type_t type, const char* str, size_t len);
pone_node* PVIP_node_new_number(PVIPParserContext* parser, PVIP_node_type_t type, const char *str, size_t len);

void PVIP_node_push_child(pone_node* node, pone_node* child);

pone_node* PVIP_node_append_string(PVIPParserContext *parser, pone_node *node, const char* str, size_t len);
pone_node* PVIP_node_append_string_from_hex(PVIPParserContext *parser, pone_node * node, const char *str, size_t len);
pone_node* PVIP_node_append_string_from_oct(PVIPParserContext *parser, pone_node * node, const char *str, size_t len);
pone_node* PVIP_node_append_string_from_dec(PVIPParserContext *parser, pone_node * node, const char *str, size_t len);
pone_node* PVIP_node_append_string_node(PVIPParserContext *parser, pone_node*node, pone_node*stuff);
pone_node* pvip_node_alloc(struct pvip_t* pvip);

#endif /* PVIP_PRIVATE_H_ */
