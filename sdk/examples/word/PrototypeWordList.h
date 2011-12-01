#ifndef PROTOTYPEWORDLIST_H
#define PROTOTYPEWORDLIST_H

class PrototypeWordList
{
public:
    PrototypeWordList();
    static const char* pickWord(unsigned length);
    static bool isWord(const char* string);

private:
    //static const char* sList[];
};

#endif // PROTOTYPEWORDLIST_H
