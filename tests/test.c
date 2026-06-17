#include <xml.h>
static void print_indent(size_t depth) {
    for (size_t i = 0; i < depth; i++) {
        printf("  ");
    }
}

static void print_attrs(attr_array_t* attrs) {
    if (!attrs || attrs->count == 0) return;

    printf(" [");

    for (size_t i = 0; i < attrs->count; i++) {
        printf("%s=\"%s\"", attrs->data[i].name,
                            attrs->data[i].value);

        if (i + 1 < attrs->count) {
            printf(" ");
        }
    }

    printf("]");
}

static void dump_node(xml_elem_t* node, size_t depth) {
    while (node) {
        print_indent(depth);

        printf("<%s", node->name);

        print_attrs(node->attrs);

        if (!node->child) {
            printf(" />\n");
        } else {
            printf(">\n");

            dump_node(node->child, depth + 1);

            print_indent(depth);
            printf("</%s>\n", node->name);
        }

        node = node->next;
    }
}

void xml_debug_print(xml_doc_t* doc) {
    if (!doc || !doc->root) return;

    printf("XML TREE DUMP\n");
    printf("=============\n");

    dump_node(doc->root, 0);

    printf("=============\n");
}

int main(void) {

    xml_doc_t* doc = xml_doc_from_file("test.xml");

    if (!doc) {
        fprintf(stderr, "Error: could not parse XML doc from file.\n");
        return 1;
    }

    xml_debug_print(doc);

    xml_doc_destroy(doc);

    return 0;
}

