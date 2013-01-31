/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <sifteo/abi.h>
#include "event.h"
#include "cube.h"
#include "neighborslot.h"
#include "pause.h"
#include "idletimeout.h"

Event::VectorInfo Event::vectors[_SYS_NUM_VECTORS];
Event::Params Event::params[NUM_PIDS];
BitVector<Event::NUM_PIDS> Event::pending;


void Event::clearVectors()
{
    memset(vectors, 0, sizeof vectors);
}

void Event::dispatch()
{
    /*
     * Find the next event that needs to be dispatched, if any, and
     * dispatch it. In order to avoid allocating an unbounded number of
     * user-mode stack frames, this function is limited to dispatching
     * at most one event at a time.
     *
     * We dispatch events by PriorityID, which do not map 1:1 with vectors.
     * Some actions which are denoted by a PriorityID, such as re-scanning
     * neighbors, don't have a direct mapping to any single event. We also
     * choose to arrange the PriorityIDs so that CLZ will find the highest
     * priority event first, whereas the vector IDs are fixed as part of
     * our ABI.
     */

    if (!SvmRuntime::canSendEvent())
        return;

    unsigned pid;
    while (pending.findFirst(pid)) {
        Params &param = params[pid];

        switch (pid) {

            /*
             * Base Events
             */

            case PID_BASE_TRACKER:
                pending.clear(pid);
                if (callBaseEvent(_SYS_BASE_TRACKER, param.generic))
                    return;
                break;

            case PID_BASE_GAME_MENU:
                pending.clear(pid);
                if (callBaseEvent(_SYS_BASE_GAME_MENU, param.generic))
                    return;
                break;

            case PID_BASE_VOLUME_DELETE:
                pending.clear(pid);
                if (callBaseEvent(_SYS_BASE_VOLUME_DELETE, param.generic))
                    return;
                break;

            case PID_BASE_VOLUME_COMMIT:
                pending.clear(pid);
                if (callBaseEvent(_SYS_BASE_VOLUME_COMMIT, param.generic))
                    return;
                break;

            /*
             * Per-cube Events. Try to dispatch to any pending cube.
             */

            default:
                while (param.cubesPending) {
                    _SYSCubeID cid = (_SYSCubeID) Intrinsic::CLZ(param.cubesPending);
                    if (dispatchCubePID(PriorityID(pid), cid))
                        return;
                }
                pending.clear(pid);

                // Must do this AFTER clearing the 'pending' bit. This may re-raise the event.
                cubeEventsClear(PriorityID(pid));
                break;

        }
    }
}

bool Event::dispatchCubePID(PriorityID pid, _SYSCubeID cid)
{
    /*
     * Special cube events.
     *
     * Instead of sending a single idempotent event, these are typically
     * state machines that we pump on each dispatch() in order to issue
     * a single event which brings us closer to some desired state.
     *
     * These multi-part events generally clear their pending bit only
     * after they run through their state machine with no events dispatched.
     */

    switch (pid) {

        /*
         * Something neighbor-related happened, that's all we know.
         * Poke the NeighborSlot subsystem to re-scan the neighbor
         * matrix again, and produce any applicable events. We only
         * remove this from cubesPending after NeighborSlot finishes
         * generating events.
         *
         * Prevent our idle timeout from firing if a legit neighbor event
         * was observed.
         */
        case PID_NEIGHBORS:
            if (NeighborSlot::instances[cid].sendNextEvent()) {
                IdleTimeout::reset();
                return true;
            }

            Atomic::And(params[pid].cubesPending, ~Intrinsic::LZ(cid));
            return false;

        /*
         * A system cube connect/disconnect happened for this slot.
         *
         * This PID is *only* ever set pending if a system connect
         * or disconnect happened. We can determine which events
         * to send based on comparing the userConnected and sysConnected
         * flags, as well as using the disconnectFlag to catch
         * transient disconnections which haven't affected the state
         * of sysConnected at all.
         */
        case PID_CONNECTION: {
            _SYSCubeIDVector bit = Intrinsic::LZ(cid);
            _SYSCubeIDVector dcFlag = CubeSlots::disconnectFlag & bit;
            _SYSCubeIDVector userConn = CubeSlots::userConnected & bit;
            _SYSCubeIDVector sysConn = CubeSlots::sysConnected & bit;

            // Always clear disconnect flag after reading it
            Atomic::And(CubeSlots::disconnectFlag, ~bit);

            // Connected in userspace, and need disconnection?
            // Leave our event pending, since we may need to CONNECT also.
            if (userConn && (dcFlag || !sysConn)) {

                /* De-neighbor the cube just-in-time */
                _SYSNeighborState ns = NeighborSlot::instances[cid].getNeighborState();
                for(_SYSSideID i = 0; i < 4; ++i) {
                    if (ns.sides[i] < _SYS_NUM_CUBE_SLOTS && NeighborSlot::instances[ns.sides[i]].sendNextEvent()) {
                        IdleTimeout::reset();
                        return true;
                    }
                }
                if (NeighborSlot::instances[cid].sendNextEvent()) {
                    IdleTimeout::reset();
                    return true;
                }

                /*
                 * If this disconnection event takes us below the current cubeRange,
                 * execute the pause menu.
                 * Disconnect events are delivered once the game is resumed.
                 */
                if (CubeSlots::belowCubeRange())
                    Pause::mainLoop(Pause::ModeCubeRange);

                CubeSlots::instances[cid].userDisconnect();

                // flag neighbors on this and it's whole neighborhood
                return callCubeEvent(_SYS_CUBE_DISCONNECT, cid);
            }

            // Need a connect? After this point, no more events are pending for this cube.
            Atomic::And(params[pid].cubesPending, ~bit);
            if (sysConn && !userConn) {
                CubeSlots::instances[cid].userConnect();
                return callCubeEvent(_SYS_CUBE_CONNECT, cid);
            }
            return false;
        }

        default:
            break;
    }

    /*
     * Trivial cube events. These all complete in one step, so we can auto-clear
     * before the switch.
     */

    Atomic::And(params[pid].cubesPending, ~Intrinsic::LZ(cid));
    switch (pid) {
        case PID_CUBE_REFRESH:      return callCubeEvent(_SYS_CUBE_REFRESH, cid);
        case PID_CUBE_TOUCH:        return callCubeEvent(_SYS_CUBE_TOUCH, cid);
        case PID_CUBE_ASSETDONE:    return callCubeEvent(_SYS_CUBE_ASSETDONE, cid);
        case PID_CUBE_BATTERY:      return callCubeEvent(_SYS_CUBE_BATTERY, cid);
        case PID_CUBE_ACCELCHANGE:  return callCubeEvent(_SYS_CUBE_ACCELCHANGE, cid);
        default:                    ASSERT(0);
    }

    return false;
}

ALWAYS_INLINE void Event::cubeEventsClear(PriorityID pid)
{
    /*
     * We had some nonzero set of events for 'pid', and we just finished dispatching
     * them all, for all cubes.
     *
     * We use this opportunity to reset the state for stretched 'touch' events, now
     * that we know userspace has seen them.
     */

    switch (pid) {

        case PID_CUBE_TOUCH:
            return CubeSlots::clearTouchEvents();

        default:
            break;
    }
}

void Event::setBasePending(PriorityID pid, uint32_t param)
{
    Atomic::Store(params[pid].generic, param);
    pending.atomicMark(pid);
}

void Event::setCubePending(PriorityID pid, _SYSCubeID cid)
{
    ASSERT(cid < _SYS_NUM_CUBE_SLOTS);

    Atomic::SetLZ(params[pid].cubesPending, cid);
    pending.atomicMark(pid);
}

void Event::setVector(_SYSVectorID vid, void *handler, void *context)
{
    ASSERT(vid < _SYS_NUM_VECTORS);
    vectors[vid].handler = reinterpret_cast<reg_t>(handler);
    vectors[vid].context = reinterpret_cast<reg_t>(context);
}

void *Event::getVectorHandler(_SYSVectorID vid)
{
    ASSERT(vid < _SYS_NUM_VECTORS);
    return reinterpret_cast<void*>(vectors[vid].handler);
}

void *Event::getVectorContext(_SYSVectorID vid)
{
    ASSERT(vid < _SYS_NUM_VECTORS);
    return reinterpret_cast<void*>(vectors[vid].context);
}

bool Event::callBaseEvent(_SYSVectorID vid, uint32_t param)
{
    ASSERT(vid < _SYS_NUM_VECTORS);
    VectorInfo &vi = vectors[vid];

    if (vi.handler) {
        SvmRuntime::sendEvent(vi.handler, vi.context, param);
        return true;
    }

    return false;
}

bool Event::callCubeEvent(_SYSVectorID vid, _SYSCubeID cid)
{
    ASSERT(vid < _SYS_NUM_VECTORS);
    ASSERT(cid < _SYS_NUM_CUBE_SLOTS);
    VectorInfo &vi = vectors[vid];

    if (vi.handler) {
        SvmRuntime::sendEvent(vi.handler, vi.context, cid);
        return true;
    }

    return false;
}

bool Event::callNeighborEvent(_SYSVectorID vid, _SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1)
{
    ASSERT(vid < _SYS_NUM_VECTORS);
    VectorInfo &vi = vectors[vid];

    if (vi.handler) {
        SvmRuntime::sendEvent(vi.handler, vi.context, c0, s0, c1, s1);
        return true;
    }
    
    return false;
}
