#ifndef H_XML
#define H_XML

#include <stdio.h>
#include <stdlib.h>

typedef struct attr {
    char* name;
    char* value;
} attr_t;

typedef struct {
    attr_t* data;
    size_t count;
    size_t capacity;
} attr_array_t;

typedef struct {
    char* name;
    attr_array_t* attrs;
} xml_elem_t;

typedef struct {
    xml_elem_t* root;
} xml_doc_t;

xml_elem_t* xml_elem_create();
void xml_elem_destroy(xml_elem_t* elem);

xml_doc_t* xml_doc_from_file(const char* filename);

xml_doc_t* xml_doc_create();
void xml_doc_destroy(xml_doc_t* doc);

#endif // H_XML
