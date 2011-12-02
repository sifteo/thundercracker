#ifndef EVENTID_H
#define EVENTID_H

enum EventID
{
    EventID_AddNeighbor,
    EventID_RemoveNeighbor,
    EventID_Paint,
    EventID_EnterState,
    EventID_ExitState,
    EventID_RequestNewAnagram,
    EventID_NewAnagram,
    EventID_NewWordFound,
    EventID_OldWordFound,
    EventID_WordBroken,

    NumEventIds
};

#endif // EVENTID_H
