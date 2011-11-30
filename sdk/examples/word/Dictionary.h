#ifndef DICTIONARY_H
#define DICTIONARY_H

class Dictionary
{
public:
    Dictionary();

    static const char* pickWord(unsigned length);
    static bool isWord(const char* string);
    static bool isOldWord(const char* word) { return false; }
};

#endif // DICTIONARY_H
