# W3C-inspired XML Processor (C)

A work-in-progress XML processor written in C.

This project is an experimental implementation of an XML processor with a focus on understanding parsing techniques, recursive descent design, and memory management in low-level systems programming.

It is **not yet a fully W3C-compliant parser**, but it is structured with that long-term direction in mind.

---

## Current Status

The parser is in an early but functional stage:

- Basic XML tokenization and parsing are implemented
- Recursive descent parsing is used for core grammar rules
- Element tree structure (DOM-like) is being built
- Attribute parsing is implemented using a dynamic array
- Basic tree traversal and debug printing are available

However, the project is still under active development and contains known limitations:

### Known limitations

- Text nodes (`CharData`) are not fully implemented
- XML namespaces are not supported
- Error reporting is minimal (boolean-based success/failure only)
- Some edge cases in attribute parsing are still being refined
- Memory ownership rules are still being stabilized
- Parser recovery after errors is not implemented

---

## Architecture Overview

### Parsing approach

The parser is implemented as a **recursive descent parser**:

- Each grammar rule is represented by a dedicated function
- The input is consumed via a cursor-based parser (`parser_t`)
- Backtracking is performed by restoring the cursor on failure

### Example grammar rules implemented

- `element`
- `STag`
- `EmptyElemTag`
- `Attribute`
- `AttValue`
- `Name`
- `S` (whitespace)

---

### Tree structure

Parsed XML is stored as a tree:

- Each node has:
  - a name (allocated string)
  - a dynamic array of attributes
  - a pointer to its first child
  - a pointer to its next sibling

This forms a standard **first-child / next-sibling tree model**.

---

## Debug Utilities

The project includes a tree debug printer that outputs the parsed XML structure in a readable format:

Example output:
```
XML TREE DUMP
=============
<🤢 [att1="value1"]>
  <child [att2="value2"] />
  <child [att3="value3"] />
</🤢>
=============
```

---

## Notes

This project is actively evolving. Internal APIs and data structures may change significantly between versions as the parser architecture stabilizes.
