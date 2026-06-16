#ifndef H_XML
#define H_XML

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    char* name;
} xml_elem_t;

typedef struct {
    xml_elem_t* root;
} xml_doc_t;

xml_doc_t* xml_doc_from_file(const char* filename);

#endif // H_XML
