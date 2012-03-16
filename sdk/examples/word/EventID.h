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
    EventID_Shake,
    EventID_Tilt,
    EventID_NewAnagram,
    EventID_NewWordFound,
    EventID_OldWordFound,
    EventID_WordBroken,
    EventID_GameStateChanged,
    EventID_LetterOrderChange,
    EventID_UpdateHintSolution,
    EventID_HintSolutionUpdated,
    EventID_SpendHint,

    NumEventIds
};

#endif // EVENTID_H
