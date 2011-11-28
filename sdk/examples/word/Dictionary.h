#ifndef DICTIONARY_H
#define DICTIONARY_H

class Dictionary
{
public:
    Dictionary();

    static const char* pickWord(unsigned length);
    static bool isWord(const char* string);
};

#endif // DICTIONARY_H
