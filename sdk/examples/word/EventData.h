#ifndef EVENTDATA_H
#define EVENTDATA_H

union EventData
{
    EventData() {}
    struct
    {
        const char* mWord;
        int mOffLengthIndex;
    } mNewAnagram;

    struct
    {
        bool mFirst;
    } mEnterState;
};

#endif // EVENTDATA_H
