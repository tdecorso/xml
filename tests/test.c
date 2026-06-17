#include <xml.h>

// XMLDecl ::= '<?xml' VersionInfo EncodingDecl? SDDecl? S? '?>'

int main(void) {

    xml_doc_t* doc = xml_doc_from_file("../books.xml");

    if (!doc) {
        fprintf(stderr, "Error: could not parse XML doc from file.\n");
        return 1;
    }

    printf("Root name: %s\n", doc->root->name);
    if (doc->root->attrs->count > 0) {
        printf("Root attributes: \n");
        for (size_t i = 0; i < doc->root->attrs->count; i++) {
            printf("  %s=\"%s\"\n", doc->root->attrs->data[i].name,
                                    doc->root->attrs->data[i].value);
        }
    }

    xml_doc_destroy(doc);

    return 0;
}

