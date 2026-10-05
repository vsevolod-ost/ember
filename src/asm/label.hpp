// label.hpp — the assembler's labels: a linked list, like the tokens. How
// many labels a program has is known only once it has been read.
#pragma once

#include <cstdint>
#include <string>

struct Label {
    std::string name;
    std::uint16_t addr; // the address of the byte after `name:`
    int line;           // where it is defined, for "already defined on line N"
    Label* next;        // nullptr at the end
};

// Add `name` at the end (the order of definition); the list grows by one node.
void label_add(Label*& head, const std::string& name, std::uint16_t addr, int line);

// The label called `name`; nullptr if there is none. Case matters.
const Label* label_find(const Label* head, const std::string& name);

// The first label (in the order of definition) at `addr`; nullptr if none.
const Label* label_at(const Label* head, std::uint16_t addr);

void labels_destroy(Label* head);
