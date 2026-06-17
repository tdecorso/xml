#include <xml.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    const char* cur;
    const char* end;
} parser_t;

typedef struct {
    const char* start;
    size_t len;
} string_view_t;

attr_array_t* attr_array_create(size_t capacity) {
    attr_array_t* arr = malloc(sizeof(attr_array_t));
    if (!arr) return NULL;
    arr->data = malloc(sizeof(attr_t) * capacity);
    if (!arr->data) {
        free(arr);
        return NULL;
    }
    arr->capacity = capacity;
    arr->count = 0;
    return arr;
}

void attr_array_destroy(attr_array_t* arr) {
    if (!arr) return;
    for (size_t i = 0; i < arr->count; i++) {
        if (arr->data[i].name) free(arr->data[i].name);
        if (arr->data[i].value) free(arr->data[i].value);
    }
    free(arr->data);
    free(arr);
}

bool attr_array_append(attr_array_t* a,
                       string_view_t name,
                       string_view_t value) {
    if (!a) return false;

    if (a->count == a->capacity) {
        size_t new_cap = (a->capacity == 0) ? 4 : a->capacity * 2;
        attr_t* new_data =
            realloc(a->data, new_cap * sizeof(attr_t));
        if (!new_data) return false;

        a->data = new_data;
        a->capacity = new_cap;
    }

    a->data[a->count].name = strndup(name.start, name.len);
    a->data[a->count].value = strndup(value.start, value.len);
    a->count++;

    return true;
}

xml_elem_t* xml_elem_create() {
    xml_elem_t* elem = calloc(1, sizeof(xml_elem_t));
    if (!elem) return NULL;
    return elem;
}

void xml_elem_destroy(xml_elem_t* elem) {
    if (!elem) return;
    attr_array_destroy(elem->attrs);
    if (elem->name) free(elem->name);
    xml_elem_destroy(elem->child);
    xml_elem_destroy(elem->next);
    free(elem);
}

xml_doc_t* xml_doc_create() {
    xml_doc_t* doc = calloc(1, sizeof(xml_doc_t));
    if (!doc) return NULL;
    return doc;
}

void xml_doc_destroy(xml_doc_t* doc) {
    if (!doc) return;
    xml_elem_destroy(doc->root);
    free(doc);
}

static char peek(parser_t* p) {
    if (!p) return '\0';
    if (p->cur >= p->end) return '\0';
    return *p->cur;
}

static void adv(parser_t* p) {
    if (!p) return;
    if (p->cur < p->end) p->cur++;
}

static void advn(parser_t* p, size_t n) {
    if (!p) return;
    if (p->cur + n <= p->end) p->cur += n;
}

static bool consume(parser_t* p, char c) {
    if (!p) return false;
    char next = peek(p);
    if (next == c) {
        adv(p);
        return true;
    }
    return false;
}

static char* read_file(const char* filename, size_t* size_out) {
    if (!filename) return 0;
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    size_t total_size = ftell(f);
    rewind(f);
    if (total_size == 0) {
        fclose(f);
        return NULL;
    }

    char* buf = malloc(total_size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(buf, 1, total_size, f);
    if (read < total_size) {
        fclose(f);
        return NULL;
    }

    buf[read] = '\0';
    fclose(f);

    if (size_out) *size_out = read;

    return buf;
}

bool peek_utf8_cp(parser_t* p, uint32_t* cp, size_t* n) {
    if (!p) return false;
    if (!cp) return false;
    if (!n) return false;
    uint8_t c0 = (uint8_t) p->cur[0];
    if (c0 <= 0x7F) {
        *cp = c0;
        *n = 1;
        return true;
    }
    if ((c0 & 0xE0) == 0xC0) {
        if (p->cur + 1 >= p->end) return false;
        uint8_t c1 = (uint8_t) p->cur[1];
        if ((c1 & 0xC0) != 0x80) return false;
        *cp = ((c0 & 0x1F) << 6) | (c1 & 0x3F);
        *n = 2;
        return true;
    }
    if ((c0 & 0xF0) == 0xE0) {
        if (p->cur + 2 >= p->end) return false;
        uint8_t c1 = (uint8_t) p->cur[1];
        uint8_t c2 = (uint8_t) p->cur[2];
        if ((c1 & 0xC0) != 0x80) return false;
        if ((c2 & 0xC0) != 0x80) return false;
        *cp = ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
        *n = 3;
        return true;
    }
    if ((c0 & 0xF8) == 0xF0) {
        if (p->cur + 3 >= p->end) return false;
        uint8_t c1 = (uint8_t) p->cur[1];
        uint8_t c2 = (uint8_t) p->cur[2];
        uint8_t c3 = (uint8_t) p->cur[3];
        if ((c1 & 0xC0) != 0x80) return false;
        if ((c2 & 0xC0) != 0x80) return false;
        if ((c3 & 0xC0) != 0x80) return false;
        *cp = ((c0 & 0x07) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
        *n = 4;
        return true;
    }
    return false;
}

/* 
 * NameStartChar ::= ":" | [A-Z] | "_" | [a-z]         | 
 *                       [#xC0-#xD6] | [#xD8-#xF6]     |
 *                      [#xF8-#x2FF] | [#x370-#x37D]   |
 *                    [#x37F-#x1FFF] | [#x200C-#x200D] |
 *                   [#x2070-#x218F] | [#x2C00-#x2FEF] | 
 *                   [#x3001-#xD7FF] | [#xF900-#xFDCF] | 
 *                   [#xFDF0-#xFFFD] | [#x10000-#xEFFFF]
 */
bool parse_NameStartChar(parser_t* p) {
    uint32_t cp;
    size_t n;
    if (!peek_utf8_cp(p, &cp, &n)) return false;
    bool is_valid = cp == ':' ||
                    cp == '_' ||
                    (cp >= 'A'     && cp <= 'Z')     ||
                    (cp >= 'a'     && cp <= 'z')     ||
                    (cp >= 0x000C0 && cp <= 0x000D6) ||
                    (cp >= 0x000D8 && cp <= 0x000F6) ||
                    (cp >= 0x000F8 && cp <= 0x002FF) ||
                    (cp >= 0x00370 && cp <= 0x0037D) ||
                    (cp >= 0x0037F && cp <= 0x01FFF) ||
                    (cp >= 0x0200C && cp <= 0x0200D) ||
                    (cp >= 0x02070 && cp <= 0x0218F) ||
                    (cp >= 0x02C00 && cp <= 0x02FEF) ||
                    (cp >= 0x03001 && cp <= 0x0D7FF) ||
                    (cp >= 0x0F900 && cp <= 0x0FDCF) ||
                    (cp >= 0x0FDF0 && cp <= 0x0FFFD) ||
                    (cp >= 0x10000 && cp <= 0xEFFFF);

    if (is_valid) {
        advn(p, n);
        return true;
    }
    return false;
}

bool parse_NameChar(parser_t* p) {
    uint32_t cp;
    size_t n;
    if (!peek_utf8_cp(p, &cp, &n)) return false;
    bool is_valid = cp == ':'  ||
                    cp == '_'  || 
                    cp == '-'  || 
                    cp == '.'  ||
                    cp == 0xB7 ||
                    (cp >= 'A'     && cp <= 'Z')     ||
                    (cp >= 'a'     && cp <= 'z')     ||
                    (cp >= '0'     && cp <= '9')     ||
                    (cp >= 0x00300 && cp <= 0x0036F) ||
                    (cp >= 0x0203F && cp <= 0x02040) ||
                    (cp >= 0x000C0 && cp <= 0x000D6) ||
                    (cp >= 0x000D8 && cp <= 0x000F6) ||
                    (cp >= 0x000F8 && cp <= 0x002FF) ||
                    (cp >= 0x00370 && cp <= 0x0037D) ||
                    (cp >= 0x0037F && cp <= 0x01FFF) ||
                    (cp >= 0x0200C && cp <= 0x0200D) ||
                    (cp >= 0x02070 && cp <= 0x0218F) ||
                    (cp >= 0x02C00 && cp <= 0x02FEF) ||
                    (cp >= 0x03001 && cp <= 0x0D7FF) ||
                    (cp >= 0x0F900 && cp <= 0x0FDCF) ||
                    (cp >= 0x0FDF0 && cp <= 0x0FFFD) ||
                    (cp >= 0x10000 && cp <= 0xEFFFF);

    if (is_valid) {
        advn(p, n);
        return true;
    }
    return false;
}


// Name ::= NameStartChar (NameChar)*
bool parse_Name(parser_t* p, string_view_t* name) {
    if (!p) return false;
    if (!name) return false;
    const char* start = p->cur;
    if (!parse_NameStartChar(p)) return false;
    name->start = start;
    while (true) {
        if (!parse_NameChar(p)) break;
    }
    name->len = p->cur - name->start;
    return true;
}

// S ::= (#x20 | #x9 | #xD | #xA)+
bool parse_S(parser_t* p) {
    if (!p) return false;
    const char* start = p->cur;
    while (true) {
        char next = peek(p);
        if (next == '\0') break;
        if (next != 0x20 && 
            next != 0x09 &&
            next != 0x0D &&
            next != 0x0A) break;
        adv(p);
    }
    return start != p->cur;
}

// Eq ::= S? '=' S?
bool parse_Eq(parser_t* p) {
    if (!p) return false;
    const char* start = p->cur;
    parse_S(p);
    if (!consume(p, '=')) {
        p->cur = start;
        return false;
    }
    parse_S(p);
    return true;
}

// AttValue ::= '"' ([^<&"] | Reference)* '"' |  "'" ([^<&'] | Reference)* "'"
bool parse_AttValue(parser_t* p, string_view_t* value) {
    if (!p) return false;

    const char* start = p->cur;
    char quote = peek(p) == '"' ? '"' : '\'';

    if (!consume(p, quote)) {
        p->cur = start;
        return false;
    }

    const char* value_start = p->cur;

    while (true) {
        char next = peek(p);
        if (next == '\0') {
            p->cur = start;
            return false;
        }
        if (next == '<' || next == '&') {
            p->cur = start;
            return false;
        }
        if (next == quote) break;
        adv(p);
    }


    if (!consume(p, quote)) {
        p->cur = start;
        return false;
    }

    value->start = value_start;
    value->len = p->cur - value_start - 1;
    return true;
}

// Attribute ::= Name Eq AttValue
bool parse_Attribute(parser_t* p, string_view_t* name, string_view_t* value) {
    if (!p) return false;
    const char* start = p->cur;
    if (!parse_Name(p, name)) {
        p->cur = start;
        return false;
    }
    if (!parse_Eq(p)) {
        p->cur = start;
        return false;
    }
    if (!parse_AttValue(p, value)) {
        p->cur = start;
        return false;
    }
    return true;
}

void attr_array_clear(attr_array_t* arr) {
    if (!arr) return;
    for (size_t i = 0; i < arr->count; i++) {
        if (arr->data[i].name) free(arr->data[i].name);
        if (arr->data[i].value) free(arr->data[i].value);
    }
    arr->count = 0;
}

// EmptyElemTag ::= '<' Name (S Attribute)* S? '/>'
bool parse_EmptyElemTag(parser_t* p, string_view_t* name, attr_array_t* attrs) {
    if (!p || !name || !attrs) return false;

    const char* start = p->cur;
    if (!consume(p, '<')) {
        return false; 
    }
    if (!parse_Name(p, name)) {
        p->cur = start;
        return false;
    }


    while (true) {
        const char* beforeS = p->cur;
        if (!parse_S(p)) break;

        string_view_t attname;
        string_view_t attvalue;

        if (!parse_Attribute(p, &attname, &attvalue)) {
            p->cur = beforeS;
            break;
        }

        if (!attr_array_append(attrs, attname, attvalue)) {
            p->cur = start;
            return false;
        }
    }

    parse_S(p);
    if (!consume(p, '/') || !consume(p, '>')) {
        p->cur = start;
        attr_array_clear(attrs);
        return false;
    }

    return true;
}

// STag ::= '<' Name (S Attribute)* S? '>'
bool parse_STag(parser_t* p, string_view_t* name, attr_array_t* attrs) {
    const char* start = p->cur;
    if (!consume(p, '<')) {
        return false; 
    }
    if (!parse_Name(p, name)) {
        p->cur = start;
        return false;
    }
    while (true) {
        const char* beforeS = p->cur;
        if (!parse_S(p)) break;

        string_view_t attname;
        string_view_t attvalue;

        if (!parse_Attribute(p, &attname, &attvalue)) {
            p->cur = beforeS;
            break;
        }

        if (!attr_array_append(attrs, attname, attvalue)) {
            p->cur = start;
            return false;
        }
    }

    parse_S(p);
    if (!consume(p, '>')) {
        attr_array_clear(attrs);
        p->cur = start;
        return false;
    }

    return true;
}

static xml_elem_t* parse_element(parser_t* p);

// content ::= CharData? ((element | Reference | CDSect | PI | Comment) CharData?)*
bool parse_content(parser_t* p, xml_elem_t* parent) {
    if (!p) return false;
    if (!parent) return false;
    parse_S(p);
    while (true) {
        xml_elem_t* elem = parse_element(p);
        if (!elem) break;
        if (parent->child == NULL) {
            parent->child = elem;
        }
        else {
            xml_elem_t* iter = parent->child;
            while (iter->next != NULL) {
                iter = iter->next;
            }
            iter->next = elem;
        }
        parse_S(p);
    }
    parse_S(p);
    
    return true;
}

// ETag ::= '</' Name S? '>'
bool parse_ETag(parser_t* p, string_view_t expected) {
    if (!p) return false;
    const char* start = p->cur;
    if (!consume(p, '<') || !consume(p, '/')) {
        p->cur = start;
        return false;
    }
    string_view_t name;
    if (!parse_Name(p, &name)) {
        p->cur = start;
        return false;
    }
    parse_S(p);
    if (!consume(p, '>')) {
        p->cur = start;
        return false;
    }
    if (expected.len != name.len) return false;
    if (strncmp(expected.start, name.start, name.len) != 0) return false;
    return true;
}

// element ::= EmptyElemTag | STag content ETag
static xml_elem_t* parse_element(parser_t* p) {
    if (!p) return NULL;
    const char* start = p->cur;
    string_view_t name;
    attr_array_t* attrs = attr_array_create(8);
    if (parse_EmptyElemTag(p, &name, attrs)) {
        xml_elem_t* elem = xml_elem_create();
        elem->name = strndup(name.start, name.len);
        elem->attrs = attrs;
        return elem;
    }
    if (parse_STag(p, &name, attrs)) {
        xml_elem_t* elem = xml_elem_create();
        elem->name = strndup(name.start, name.len);
        elem->attrs = attrs;
        if (!parse_content(p, elem) || !parse_ETag(p, name)) {
            xml_elem_destroy(elem);
            p->cur = start;
            return NULL;
        }
        return elem;
    }
    attr_array_destroy(attrs);
    return NULL;
}

xml_doc_t* xml_doc_from_file(const char* filename) {
    size_t len;
    char* xml = read_file(filename, &len);
    if (!xml) return NULL;

    parser_t p = {xml, xml + len};

    xml_elem_t* root =  parse_element(&p);

    if (!root) {
        free(xml);
        return NULL;
    }

    xml_doc_t* doc = xml_doc_create();
    if (!doc) {
        free(xml);
        return NULL;
    }

    doc->root = root;

    free(xml);
    return doc;
}
