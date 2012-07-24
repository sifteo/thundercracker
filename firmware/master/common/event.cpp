/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <sifteo/abi.h>
#include "event.h"
#include "cube.h"
#include "neighborslot.h"

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
         */
        case PID_NEIGHBORS:
            if (NeighborSlot::instances[cid].sendNextEvent())
                return true;
            Atomic::And(params[pid].cubesPending, ~Intrinsic::LZ(cid));
            return false;

        /*
         * A system cube connect/disconnect happened for this slot.
         *
         * This PID is *only* ever set pending if a system connect
         * or disconnect happened. So, we can guess what happened
         * based on the current state of the sysConnected and
         * userConnected bits for this cube.
         *
         * If the two bits are in different states, a single connect
         * or disconnect occurred on this cube. If both are 0, a
         * cube connected but subsequently disconnected before we
         * could tell userspace about it. Ignore these events. If
         * both are 1, however, this means a cube that was previously
         * connected has disconnected, and a (possibly different)
         * cube connected in its place.
         *
         * For this last case, we want to emit a disconnect event
         * followed by a connect. To do this in two steps, we'll
         * emit the disconnect and clear the userConnected bit
         * without clearing the cubesPending bit.
         */
        case PID_CONNECTION: {
            _SYSCubeIDVector bit = Intrinsic::LZ(cid);
            _SYSCubeIDVector userConn = CubeSlots::userConnected & bit;
            _SYSCubeIDVector sysConn = CubeSlots::sysConnected & bit;

            if (userConn) {
                if (sysConn) {
                    // Disconnect followed by reconnect (Leave event pending)
                    Atomic::And(CubeSlots::userConnected, ~bit);
                    return callCubeEvent(_SYS_CUBE_DISCONNECT, cid);
                } else {
                    // Disconnect only
                    Atomic::And(params[pid].cubesPending, ~bit);
                    Atomic::And(CubeSlots::userConnected, ~bit);
                    return callCubeEvent(_SYS_CUBE_DISCONNECT, cid);
                }
            } else {
                if (sysConn) {
                    // Connect only
                    Atomic::And(params[pid].cubesPending, ~bit);
                    Atomic::Or(CubeSlots::userConnected, bit);
                    return callCubeEvent(_SYS_CUBE_CONNECT, cid);
                } else {
                    // Connect followed by disconnect.
                    return false;
                }
            }
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
        case PID_CUBE_TILT:         return callCubeEvent(_SYS_CUBE_TILT, cid);
        case PID_CUBE_SHAKE:        return callCubeEvent(_SYS_CUBE_SHAKE, cid);
        case PID_CUBE_BATTERY:      return callCubeEvent(_SYS_CUBE_BATTERY, cid);
        case PID_CUBE_ACCELCHANGE:  return callCubeEvent(_SYS_CUBE_ACCELCHANGE, cid);
        default:                    ASSERT(0);
    }

    return true;
}

void Event::setBasePending(PriorityID pid, uint32_t param)
{
    if (pending.test(pid)) {
        LOG(("EVENT: Warning, event %d already registered (param: 0x%08X), overwriting.\n",
             pid, params[pid].generic));
    }

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
