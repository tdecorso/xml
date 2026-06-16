#include <xml.h>

// XMLDecl ::= '<?xml' VersionInfo EncodingDecl? SDDecl? S? '?>'

int main(void) {

    xml_doc_t* doc = xml_doc_from_file("../books.xml");

    if (!doc) {
        fprintf(stderr, "Error: could not parse XML doc from file.\n");
        return 1;
    }

    return 0;
}

