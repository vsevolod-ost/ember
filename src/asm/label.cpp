// label.cpp — the list of labels: push_back, walk, destroy.
#include "label.hpp"

void label_add(Label*& head, const std::string& name, std::uint16_t addr, int line) {
    Label* l = new Label{name, addr, line, nullptr};
    if (!head) {
        head = l;
        return;
    }
    Label* last = head;
    while (last->next)
        last = last->next;
    last->next = l;
}

const Label* label_find(const Label* head, const std::string& name) {
    for (const Label* l = head; l; l = l->next)
        if (l->name == name)
            return l;
    return nullptr;
}

const Label* label_at(const Label* head, std::uint16_t addr) {
    for (const Label* l = head; l; l = l->next)
        if (l->addr == addr)
            return l;
    return nullptr;
}

void labels_destroy(Label* head) {
    while (head) {
        Label* next = head->next;
        delete head;
        head = next;
    }
}
