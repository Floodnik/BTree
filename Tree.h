#ifndef _TREE_H_
#define _TREE_H_

#include <vector>
#include <array>
#include <exception>

#include "IO.h"

const int MIN_DEGREE = 2;

class excCorrupted : std::exception
{
private:
    std::string extras;

public:
    excCorrupted(const std::string& _extras);
    const char* what() const noexcept;
};

class BTree
{
private:
    struct node
    {
        unsigned prevDeleted;
        unsigned keyCount;
        unsigned name;
        bool isLeaf;
        std::array<int, 2*MIN_DEGREE-1> keys;
        std::array<unsigned, 2*MIN_DEGREE> children;
    };

    struct
    {
        unsigned rootName;
        unsigned nodeCount;
        unsigned lastDeleted;
    } info;

    const std::string name;
    IO diskIO;
    node root, current;

    template<typename T> bool save(T& data, const unsigned& suffix);
    template<typename T> bool load(T& data, const unsigned& suffix);

    void delete_node(unsigned& nodeName);
    unsigned new_node();

    void merge_children(node& parent, node& leftChild, node& rightChild, const int& index);
    node split_child(node& parent, node& fullChild, const int& index);

    void insert_nonfull(int k);
    void intern_preorder(node& x);
    bool intern_delete(node& x, int k);

public:
    BTree(const std::string& _name);
    ~BTree();
    void insert_key(int k);
    bool delete_key(int k);
    bool find_key(int k);
    void preorder();
};

#endif // _TREE_H_
