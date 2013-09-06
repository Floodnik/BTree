#include <iostream>

#include "Tree.h"

using namespace std;

excCorrupted::excCorrupted(const string& _extras)
    : extras(_extras) {};
const char* excCorrupted::what() const noexcept
{
    string retval = "Couldn't load node file. Possibly corrupted structure. " + extras;
    return retval.c_str();
}

template<typename T> bool BTree::save(T& data, const unsigned& suffix)
{
    return diskIO.write(reinterpret_cast<byte*>(&data), suffix, sizeof(data));
}

template<typename T> bool BTree::load(T& data, const unsigned& suffix)
{
    return diskIO.read(reinterpret_cast<byte*>(&data), suffix, sizeof(data));
}

BTree::BTree(const std::string& _name)
    : name(_name), diskIO(name)
{
    //if(!diskIO.read(reinterpret_cast<byte*>(&info), 0)) {
    if(!load(info, 0)) {
        cout << "Creating a new BTree." << endl;

        info.nodeCount = info.lastDeleted = 0;
        info.rootName = new_node();

        root.keyCount = 0;
        root.name = 1;
        root.isLeaf = true;

        save(info, 0);
        save(root, root.name);
    }
    else {
        cout << "Loaded an existing file." << endl;
        if(!load(root, info.rootName)) {
            throw excCorrupted("root");
        }
    }
}

BTree::~BTree()
{

}

//when we create a node we need a name for it
//if nodes were created previously, their names form a name stack for the new node's name to be taken from
//if the name queue is empty, node count is our new node's name
void BTree::delete_node(unsigned& nodeName)
{
    info.nodeCount--;
    save(info.lastDeleted, nodeName);
    info.lastDeleted = nodeName;

    save(info, 0);
}

unsigned BTree::new_node()
{
    ++info.nodeCount;
    if(info.lastDeleted) {
        unsigned nodeName = info.lastDeleted;
        load(info.lastDeleted, nodeName);
        save(info, 0);
        return nodeName;
    }
    save(info, 0);
    return info.nodeCount;
}

//during this operation index is fullChild's index among parent's children(the "children" array placement)
BTree::node BTree::split_child(node& parent, node& fullChild, const int& index)
{
    node newNode;

    newNode.isLeaf = fullChild.isLeaf;
    newNode.keyCount = MIN_DEGREE-1;
    newNode.name = new_node();

    //move MIN_DEGREE-1 greater keys from the full child to the new node
    for(int i=0; i < MIN_DEGREE-1; ++i)
        newNode.keys[i] = fullChild.keys[i+MIN_DEGREE];

    //cout << "move children" << endl;
    //also, if the full child(thus the new node) isn't a leaf, move corresponding children
    if(!fullChild.isLeaf) {
        for(int i=0; i <= MIN_DEGREE-1; ++i)
            newNode.children[i] = fullChild.children[i+MIN_DEGREE];
    }

    fullChild.keyCount = MIN_DEGREE-1;

    //make room for the new child in the parent node
    for(int i = parent.keyCount; i >= index+1; --i)
        parent.children[i+1] = parent.children[i];

    parent.children[index+1] = newNode.name;

    //make room for fullChild's old median key in the parent node
    for(int i = parent.keyCount-1; i >= index; --i) {
        cout << "i = " << i << endl;
        parent.keys[i+1] = parent.keys[i];
    }

    //move fullChild's old median key to the parent
    parent.keys[index] = fullChild.keys[MIN_DEGREE-1];

    parent.keyCount++;

    save(fullChild, fullChild.name);
    save(newNode, newNode.name);
    save(parent, parent.name);
    return newNode;
}

//during this operation index points to the key whose surrounding nodes get merged
void BTree::merge_children(node& parent, node& leftChild, node& rightChild, const int& index)
{
    int i = index;
    int k = parent.keys[index];

    //place k as the median key in our new subtree
    leftChild.keys[MIN_DEGREE-1] = k;

    //move all the keys from the right child to our new subtree
    for(int j = 0; j < MIN_DEGREE-1; ++j)
        leftChild.keys[MIN_DEGREE + j] = rightChild.keys[j];

    //if the right child(thus the left child) wasn't a leaf, move all the children
    if(!rightChild.isLeaf) {
        for(int j = 0; j < MIN_DEGREE; ++j)
            leftChild.children[MIN_DEGREE + j] = rightChild.children[j];
    }

    leftChild.keyCount = 2*MIN_DEGREE-1;

    //rightChild is no longer an used node, so its name is no longer used - put it into the "free names" queue and decrease node count
    //note that the file stays on the disk and contains previous deleted node's name
    delete_node(rightChild.name);

    //remove k from the parent, move all the children and keys to fix its structure
    ++i;
    while(i < parent.keyCount) {
        parent.keys[i-1] = parent.keys[i];
        parent.children[i] = parent.children[i+1];
        ++i;
    }
    parent.keyCount--;

    //if parent is the root and it became empty, the merged node becomes the root
    if(!parent.keyCount) {

        //parent's name is now free
        delete_node(parent.name);

        root = leftChild;
        info.rootName = root.name;
        save(info, 0);
    }
    else {
        save(parent, parent.name);
        if(root.name == parent.name)
            root = move(parent);
    }
    save(leftChild, leftChild.name);
}

void BTree::insert_nonfull(int k)
{
    current = root;
    while(true)
    {
        int i = current.keyCount-1;
        if(current.isLeaf) {
            while(i>=0 && k < current.keys[i])
            {
                current.keys[i+1] = current.keys[i];
                --i;
            }
            current.keys[i+1] = k;
            current.keyCount++;
            save(current, current.name);
            if(current.name == root.name)
                root = move(current);
            return;
        }
        while(i>=0 && k < current.keys[i])
            --i;
        i++;
        node child;
        load(child, current.children[i]);
        if(child.keyCount == 2*MIN_DEGREE-1) {
            node&& newChild = split_child(current, child, i);
            if(current.name == root.name)
                root = move(current);
            if (k > current.keys[i]) {
                current = move(newChild);
                continue;
            }
        }
        current = move(child);
    }
}

bool BTree::intern_delete(node& x, int k)
{
    //We know x has at least MIN_DEGREE keys(a condition that has to be met before descending to it) unless it's the root.

    unsigned i = 0;
    while(i < x.keyCount && k > x.keys[i])
            ++i;

    //if x is a leaf then just remove the key.
    if(x.isLeaf) {
        if(i == x.keyCount || x.keys[i] != k)
            return false;
        ++i;
        while(i < x.keyCount) {
            x.keys[i-1] = x.keys[i];
            ++i;
        }
        x.keyCount--;
        if(x.name == root.name) {
            root = move(x);
            save(root, root.name);
        }
        else
            save(x, x.name);
        return true;
    }

    //when x is an internal node and k is in it
    if(i < x.keyCount && k == x.keys[i]) {
        //if the left subtree has at least MIN_DEGREE keys
        //  delete _k - the predecessor of k in the left subtree and replace k with _k
        node leftChild;
        load(leftChild, x.children[i]);
        if(leftChild.keyCount >= MIN_DEGREE) {
            int _k = leftChild.keys[leftChild.keyCount-1];
            intern_delete(leftChild, _k);
            x.keys[i] = _k;
            if(x.name == root.name) {
                root = move(x);
                save(root, root.name);
            }
            else
                save(x, x.name);
            return true;
        }
        //if the previous didn't work, then symetrically if the right subtree has at least MIN_DEGREE keys
        //  delete _k - the successor of k in the right subtree and replace k with _k
        node rightChild;
        load(rightChild, x.children[i+1]);
        if(rightChild.keyCount >= MIN_DEGREE) {
            int _k = rightChild.keys[0];
            intern_delete(rightChild, _k);
            x.keys[i] = _k;
            if(x.name == root.name) {
                root = move(x);
                save(root, root.name);
            }
            else
                save(x, x.name);
            return true;
        }
        //else merge leftChild, k and rightChild into one subtree and delete k from it
        merge_children(x, leftChild, rightChild, i);
        intern_delete(leftChild, k);
        return true;
    }

    //x is an internal node and k is not in it
    node child;
    load(child, x.children[i]);
    //we have to make sure that the child we're going to descend to has at least MIN_DEGREE keys
    if(child.keyCount == MIN_DEGREE-1) {
            node sibling;
            bool success = false;
            if(i-1 >= 0) {
                load(sibling, x.children[i-1]);
                if(sibling.keyCount >= MIN_DEGREE) {
                    //a key moves from x(the parent) to the child and from left sibling to x

                    //make place for a new key in child
                    for(int j = child.keyCount; j > 0; --j)
                        child.keys[j] = child.keys[j-1];

                    //move key from the parent
                    child.keys[0] = x.keys[i-1];

                    //move key from sibling to parent
                    x.keys[i-1] = sibling.keys[sibling.keyCount-1];

                    //if those children are not leafs, move their children appropriately
                    if(!child.isLeaf) {
                        for(int j = child.keyCount+1; j > 0; --j)
                            child.children[j] = child.children[j-1];
                        child.children[0] = sibling.children[sibling.keyCount];
                    }

                    sibling.keyCount--;
                    child.keyCount++;
                    save(sibling, sibling.name);
                    save(child, child.name);
                    save(x, x.name);
                    success = true;
                }
            }
            if(!success && i < x.keyCount) {
                load(sibling, x.children[i+1]);
                if(sibling.keyCount >= MIN_DEGREE) {
                    //a key moves from x to the child and from right sibling to x

                    //move key from the parent
                    child.keys[child.keyCount] = x.keys[i];

                    //move key from sibling to parent
                    x.keys[i] = sibling.keys[0];

                    //fix sibling's structure
                    for(int j = 0; j < sibling.keyCount-1; ++j)
                        sibling.keys[j] = sibling.keys[j+1];

                    //if those children are not leafs, move their children appropriately
                    if(!child.isLeaf) {
                        child.children[child.keyCount+1] = sibling.children[0];
                        for(int j = 0; j < sibling.keyCount; ++j)
                            sibling.children[j] = sibling.children[j+1];
                    }

                    sibling.keyCount--;
                    child.keyCount++;
                    save(sibling, sibling.name);
                    save(child, child.name);
                    save(x, x.name);
                    success = true;
                }
            }

            if(!success) {
                    //merge child with one of his siblings
                    if(i < x.keyCount)
                        merge_children(x, child, sibling, i);
                    else {
                        merge_children(x, sibling, child, i-1);
                        return intern_delete(sibling, k);
                    }

            }
    }
    return intern_delete(child, k);
}

void BTree::intern_preorder(node& x)
{
    cout << "node " << x.name;
    if(x.isLeaf) {
        cout << ": leaf" << endl;
        for(unsigned i=0; i<x.keyCount; ++i)
            cout << x.keys[i] << " ";
        cout << endl;
    }
    else {
        cout << endl;
        for(unsigned i=0; i<x.keyCount; ++i)
            cout << "p" << x.children[i] << " " << x.keys[i] << " ";
        cout << "p" << x.children[x.keyCount] << endl;

        for(unsigned i=0; i<x.keyCount+1; ++i)
        {
            node next;
            load(next, x.children[i]);
            intern_preorder(next);
        }
    }
}

void BTree::insert_key(int k)
{
    if (root.keyCount == 2*MIN_DEGREE-1) {
        current = root;

        root.isLeaf = false;
        root.keyCount = 0;
        root.name = new_node();
        root.children[0] = current.name;

        info.rootName = root.name;
        save(info, 0);

        split_child(root, current, 0);
    }
    insert_nonfull(k);
}

bool BTree::find_key(int k)
{
    current = root;
    while(true)
    {
        unsigned i = 0;
        while(i < current.keyCount && k > current.keys[i])
            ++i;
        if(i < current.keyCount && k == current.keys[i])
            return true;
        if(current.isLeaf)
            return false;
        load(current, current.children[i]);
    }
}

bool BTree::delete_key(int k)
{
    return intern_delete(root, k);
}

void BTree::preorder()
{
    intern_preorder(root);
}


