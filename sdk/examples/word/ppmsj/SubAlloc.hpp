/****************************************************************************
 *  This file is part of PPMs project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 2004, 2006   *
 *  Contents: memory allocation routines                                    *
 ****************************************************************************/

inline void PrefetchData(void* Addr)
{
#if defined(_USE_PREFETCHING)
    BYTE PrefetchByte = *(volatile BYTE*) Addr;
#endif /* defined(_USE_PREFETCHING) */
}
enum { N1=4, N2=4, N3=4, N4=(128+3-1*N1-2*N2-3*N3)/4, N_INDEXES=N1+N2+N3+N4 };
#if (_MEM_CONFIG == 1024)
#define N_UNITS (512*1024/8)
#else
#define N_UNITS (_MEM_CONFIG*1024/8)
#endif /* _MEM_CONFIG == 1024 */
enum { TEXT_BUF_SIZE=_MEM_CONFIG*64 };
static _THREAD1 BYTE _THREAD TextBuf[TEXT_BUF_SIZE];
static _THREAD1 int  _THREAD TextIndx;

_THREAD1 UINT _THREAD NCutOffs;             // exported variable
/* constants, they are initialized at startup by PPMd_StartUp               */
static BYTE NS2BSIndx[256], QTable[260], Indx2Units[N_INDEXES], Units2Indx[128];

static _THREAD1 struct S_NODE {
    WORD Stamp, NU, wNext, wPrev;
    BOOL         avail(UINT Pos) const { return (wNext != Pos); }
    BOOL canMerge() const { return (((*(DWORD*)this) & 0x0080FFFF) == 0xFFFF); }
    inline void insert(UINT HPos,UINT Pos);
    inline UINT unlink();
    inline void remove();
} _THREAD SUnits[N_UNITS+1];
inline void S_NODE::insert(UINT HPos,UINT Pos) {
    S_NODE* p=SUnits+Pos;                   *(DWORD*) p = *(DWORD*) this;
    p->wPrev=HPos;                          p->wNext=wNext;
    wNext=SUnits[p->wNext].wPrev=Pos;
}
inline UINT S_NODE::unlink() {
    UINT Pos=wPrev;                         wPrev=SUnits[Pos].wPrev;
    SUnits[wPrev].wNext=SUnits[Pos].wNext;  return Pos;
}
inline void S_NODE::remove() {
    SUnits[wPrev].wNext=wNext;              SUnits[wNext].wPrev=wPrev;
}
static void _FASTCALL FreeUnitsRare(UINT Pos,UINT NU)
{
    while ( SUnits[Pos+NU].canMerge() ) {
        SUnits[Pos+NU].remove();            NU += SUnits[Pos+NU].NU;
    }
    for ( ;NU > 128;Pos += 128, NU -= 128)  SUnits[N_INDEXES-1].insert(N_INDEXES-1,Pos);
    UINT n, i=Units2Indx[NU-1];
    if (NU != Indx2Units[i]) {
        n=Indx2Units[--i];                  SUnits[NU-n-1].insert(NU-n-1,Pos+n);
    }
    SUnits[i].insert(i,Pos);
}
inline void FreeUnits(UINT Pos,UINT NU) {
    UINT i=Units2Indx[NU-1];                NU=Indx2Units[i];
    if ( !SUnits[Pos+NU].canMerge() )       SUnits[i].insert(i,Pos);
    else                                    FreeUnitsRare(Pos,NU);
}
static UINT _FASTCALL AllocUnitsRare(UINT indx)
{
    UINT Pos, i=indx;
    do {
        if (++i == N_INDEXES)               return 0;
    } while ( !SUnits[i].avail(i) );
    Pos=SUnits[i].unlink();
    FreeUnitsRare(Pos+Indx2Units[indx],Indx2Units[i]-Indx2Units[indx]);
    return Pos;
}
inline UINT AllocUnits(UINT NU)
{
    UINT i=Units2Indx[NU-1];
    return (SUnits[i].avail(i))?(SUnits[i].unlink()):(AllocUnitsRare(i));
}
inline void UnitsCpy(UINT DestPos,UINT SrcPos,UINT NU)
{
    DWORD* p1=(DWORD*) (SUnits+DestPos), * p2=(DWORD*) (SUnits+SrcPos);
    do {
        p1[0]=p2[0];                        p1[1]=p2[1];
        p1 += 2;                            p2 += 2;
    } while ( --NU );
}
inline UINT ExpandUnits(UINT OldPos,UINT OldNU)
{
    UINT i0=Units2Indx[OldNU-1], i1=Units2Indx[OldNU-1+1];
    if (i0 == i1)                           return OldPos;
    UINT Pos=AllocUnits(OldNU+1);
    if (Pos) { UnitsCpy(Pos,OldPos,OldNU);  FreeUnits(OldPos,OldNU); }
    return Pos;
}
inline UINT ShrinkUnits(UINT OldPos,UINT OldNU,UINT NewNU)
{
    UINT Pos, i0=Units2Indx[OldNU-1], i1=Units2Indx[NewNU-1];
    if (i0 == i1)                           return OldPos;
    if ( SUnits[i1].avail(i1) ) {
        UnitsCpy(Pos=SUnits[i1].unlink(),OldPos,NewNU);
        FreeUnits(OldPos,OldNU);            return Pos;
    } else {
        FreeUnitsRare(OldPos+Indx2Units[i1],Indx2Units[i0]-Indx2Units[i1]);
        return OldPos;
    }
}
inline UINT MoveUnits(UINT Pos,UINT NU)
{
    PrefetchData(SUnits+Pos);
    UINT NewPos, i=Units2Indx[NU-1], NewNU=Indx2Units[i];
    if (NewNU != 128 && SUnits[Pos+NewNU].canMerge() && SUnits[i].avail(i)) {
        UnitsCpy(NewPos=SUnits[i].unlink(),Pos,NU);
        FreeUnitsRare(Pos,NewNU);           return NewPos;
    }
    return Pos;
}

struct PPM_CONTEXT;
#if (_MEM_CONFIG == 1024)
static _THREAD1 struct C_NODE {
    C_NODE* Next;
#if !defined(_USE_64BIT_POINTERS)
    DWORD Dummy;
#endif /* !defined(_USE_64BIT_POINTERS) */
} _THREAD CUnits[65536];
inline PPM_CONTEXT* GetCont(UINT NU) { return (PPM_CONTEXT*) (CUnits+NU); }
inline UINT Cont2Pos(PPM_CONTEXT* pc) { return ((C_NODE*)pc)-CUnits; }
inline UINT AllocContext() {
    C_NODE* p=CUnits->Next;                 CUnits->Next=p->Next;
    return (p-CUnits);
}
inline void FreeContext(void* pv) {
    C_NODE* p=(C_NODE*) pv;                 p->Next=CUnits->Next;
    CUnits->Next=p;
}
#else
inline PPM_CONTEXT* GetCont(UINT NU) { return (PPM_CONTEXT*) (SUnits+NU); }
inline UINT Cont2Pos(PPM_CONTEXT* pc) { return ((S_NODE*)pc)-SUnits; }
inline UINT AllocContext() { return AllocUnits(1); }
inline void FreeContext(void* pv) { FreeUnits(((S_NODE*) pv)-SUnits,1); }
#endif /* _MEM_CONFIG == 1024 */

inline void InitSubAllocator()
{
    int i;                                  TextIndx=1;
#if (_MEM_CONFIG == 1024)
    for (CUnits->Next=CUnits, i=65536;--i != 0; ) {
        CUnits[i].Next=CUnits->Next;        CUnits->Next=CUnits+i;
    }
#endif /* _MEM_CONFIG == 1024 */
    for (SUnits[N_UNITS].Stamp=NCutOffs=i=0;i < N_INDEXES;i++) {
        SUnits[i].Stamp=0xFFFF;             SUnits[i].NU=Indx2Units[i];
        SUnits[i].wNext=SUnits[i].wPrev=i;
    }
    for ( ;i < 128;i++)                     SUnits[0].insert(0,i);
    for ( ;i < N_UNITS;i += 128)            SUnits[N_INDEXES-1].insert(N_INDEXES-1,i);
}
