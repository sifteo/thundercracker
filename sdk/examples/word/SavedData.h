#ifndef SAVEDDATA_H
#define SAVEDDATA_H

union EventData;

class SavedData
{
public:
    SavedData();
    static void sOnEvent(unsigned eventID, const EventData& data);

    static unsigned sHighScores[3];
};

#endif // SAVEDDATA_H
