//
// Created by dinhd on 12/17/2025.
//

#ifndef HUFFMANENCRYPT_HUFFMANCOMMON_H
#define HUFFMANENCRYPT_HUFFMANCOMMON_H

struct Node {
    char ch;
    int freq;
    Node *left, *right;

    Node(char ch, int freq) {
        left = right = nullptr;
        this->ch = ch;
        this->freq = freq;
    }
};
struct compare {
    bool operator()(Node* l, Node* r) {
        return l->freq > r->freq;
    }
};



#endif //HUFFMANENCRYPT_HUFFMANCOMMON_H