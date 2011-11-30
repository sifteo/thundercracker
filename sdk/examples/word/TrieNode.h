#ifndef TRIENODE_H
#define TRIENODE_H

struct TrieNode
{
    char mLetter;
    unsigned char mNumChildren;
    struct TrieNode* mChildren;
};

#endif // TRIENODE_H
