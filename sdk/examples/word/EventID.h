#ifndef EVENTID_H
#define EVENTID_H

enum EventID
{
    EventID_AddNeighbor,
    EventID_RemoveNeighbor,
    EventID_Paint,
    EventID_ClockTick,
    EventID_EnterState,
    EventID_ExitState,
    EventID_Input,
    EventID_NewAnagram,
    EventID_NewWordFound,
    EventID_OldWordFound,
    EventID_WordBroken,
    EventID_NewRound,
    EventID_EndRound,
    EventID_Title,

    NumEventIds
};

#endif // EVENTID_H
