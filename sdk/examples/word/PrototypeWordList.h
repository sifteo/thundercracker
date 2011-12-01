#ifndef PROTOTYPEWORDLIST_H
#define PROTOTYPEWORDLIST_H

class PrototypeWordList
{
public:
    PrototypeWordList();
    static const char* pickWord(unsigned length) {return 0;}
    static bool isWord(const char* string) {return false;}

private:
    static const char* list[];
};

#endif // PROTOTYPEWORDLIST_H
