#ifndef TRIENODE_H
#define TRIENODE_H

#include <sifteo.h>

class TrieNode
{
public:
    bool findWord(const char* word)
    {
        TrieNode* node = this;
        const char* letter = word;
        while (*letter != '\0' && node->mBitfield.mNumChildren > 0)
        {
            ASSERT(node);
            ASSERT(letter);

            bool foundMatch = false;
            // TODO binary search
            for (unsigned char i = 0; i < node->mBitfield.mNumChildren; ++i)
            {
                if (node->mChildren[i].mLetter == *letter)
                {
                    node = &node->mChildren[i];
                    ++letter;
                    // letter is now one past last node matched
                    foundMatch = true;
                    break;
                }
            }

            if (!foundMatch)
            {
                break;
            }
        }
        // true if reached null letter (by matching all previous letters)
        // and the final matching letter is a terminator (end of word)
        return *letter == '\0' && node->mBitfield.mIsTerminator;
    }

    char mLetter;
    struct Bitfield
    {
        unsigned char mNumChildren : 5;
        bool mIsTerminator : 1;
    } mBitfield;
    TrieNode* mChildren;
};

#endif // TRIENODE_H
