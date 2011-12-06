/****************************************************************************
 *  This file is part of PPMs project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 2004, 2006   *
 *  Contents: PPMII model description and encoding/decoding routines        *
 ****************************************************************************/
#include <string.h>
#include "PPMd.h"
#pragma hdrstop
#include "Coder.hpp"
#include "SubAlloc.hpp"

enum { UP_FREQ=5, INT_BITS=7, PERIOD_BITS=7, TOT_BITS=INT_BITS+PERIOD_BITS,
    INTERVAL=1 << INT_BITS, BIN_SCALE=1 << TOT_BITS, ROUND=16, MAX_FREQ=124 };

#pragma pack(1)
static _THREAD1 struct SEE2_CONTEXT { // SEE-contexts for PPM-contexts with masked symbols
    WORD Summ;
    BYTE Shift, Count;
    void init(UINT InitVal) { Summ=InitVal << (Shift=PERIOD_BITS-4); Count=7; }
    UINT getMean() {
        UINT RetVal=(Summ >> Shift);        Summ -= RetVal;
        return RetVal+!RetVal;
    }
    void update() { if (--Count == 0)       setShift_rare(); }
    void setShift_rare();
} _THREAD SEE2Cont[23][32], DummySEE2Cont;
struct STATE {
    BYTE Symbol, Freq;
    WORD wSuccessor;
    UINT               f() const { return (Freq & 0x7F); }
    BOOL         hasSucc() const { return ((Freq & 0x80) != 0); }
    PPM_CONTEXT* getSucc() const { return GetCont(wSuccessor); }
};
static _THREAD1 struct PPM_CONTEXT {        // Notes:
    BYTE NumStats, Flags;                   // 1. NumStats & NumMasked contain
    WORD SummFreq, wStats, wSuffix;         //  number of symbols minus 1
    inline void encodeBinSymbol(int symbol);// 2. sizeof(WORD) == 2*sizeof(BYTE)
    inline void   encodeSymbol1(int symbol);// 3. contexts example:
    inline void   encodeSymbol2(int symbol);// MaxOrder:
    inline void           decodeBinSymbol();//  ABCD    context
    inline void             decodeSymbol1();//   BCD    suffix
    inline void             decodeSymbol2();//   BCDE   successor
    inline void           update1(STATE* p);// other orders:
    inline void           update2(STATE* p);//   BCD    context
    inline SEE2_CONTEXT*     makeEscFreq2();//    CD    suffix
    void                          rescale();//   BCDE   successor
    BOOL _STDCALL         cutOff(int Order);
    STATE&   oneState() const { return (STATE&) SummFreq; }
    PPM_CONTEXT* suff() const { return GetCont(wSuffix); }
    STATE*   getStats() const { return (STATE*) (SUnits+wStats); }
} *_THREAD MaxContext;
#pragma pack()

static _THREAD1 STATE* _THREAD FoundState;   // found next state transition
static _THREAD1 int  _THREAD BSumm, _THREAD OrderFall, _THREAD RunLength;
static _THREAD1 int  _THREAD InitRL, _THREAD MaxOrder;
static _THREAD1 BYTE _THREAD CharMask[256], _THREAD NumMasked;
static _THREAD1 BYTE _THREAD PrevSuccess, _THREAD EscCount, _THREAD PrintCount;
static _THREAD1 WORD _THREAD BinSumm[21][64];// binary SEE-contexts

void SEE2_CONTEXT::setShift_rare()
{
    UINT i=Summ >> Shift;
    i=PERIOD_BITS-(i > 40)-(i > 280)-(i > 1020);
         if (i < Shift) { Summ >>= 1;     Shift--; }
    else if (i > Shift) { Summ <<= 1;     Shift++; }
    Count=6 << Shift;
}
static struct PPMD_STARTUP { inline PPMD_STARTUP(); } const PPMd_StartUp;
inline PPMD_STARTUP::PPMD_STARTUP()         // constants initialization
{
    UINT i, k, m, Step;
    for (i=0,k=1;i < N1     ;i++,k += 1)    Indx2Units[i]=k;
    for (k++;i < N1+N2      ;i++,k += 2)    Indx2Units[i]=k;
    for (k++;i < N1+N2+N3   ;i++,k += 3)    Indx2Units[i]=k;
    for (k++;i < N1+N2+N3+N4;i++,k += 4)    Indx2Units[i]=k;
    for (k=i=0;k < 128;k++) {
        i += (Indx2Units[i] < k+1);         Units2Indx[k]=i;
    }
    NS2BSIndx[0]=2*0;                       NS2BSIndx[1]=NS2BSIndx[2]=2*1;
    memset(NS2BSIndx+3,2*2,26);             memset(NS2BSIndx+29,2*3,256-29);
    for (i=0;i < UP_FREQ;i++)               QTable[i]=i;
    for (m=i=UP_FREQ, k=Step=1;i < 260;i++) {
        QTable[i]=m;
        if ( !--k ) { k = ++Step;           m++; }
    }
}
inline void StateCpy(STATE& s1,const STATE& s2) { (DWORD&) s1=(DWORD&) s2; }
inline void SWAP(STATE& s1,STATE& s2) {
    STATE t;                                StateCpy(t,s1);
    StateCpy(s1,s2);                        StateCpy(s2,t);
}
static void _STDCALL StartModelRare(int MaxOrder)
{
    int i, k, s;
    BYTE i2f[21];
    InitSubAllocator();
    memset(CharMask,0,sizeof(CharMask));    EscCount=PrintCount=1;
    OrderFall=::MaxOrder=MaxOrder;          RunLength=InitRL=-(MaxOrder-1);
    MaxContext=GetCont(AllocContext());     MaxContext->wSuffix=0;
    MaxContext->SummFreq=(MaxContext->NumStats=255)+2;
    MaxContext->wStats=AllocUnits(256/2);   STATE* Stats=MaxContext->getStats();
    for (PrevSuccess=i=0;i < 256;i++) {
        Stats[i].Symbol=i;                  Stats[i].Freq=1;
        Stats[i].wSuccessor=0;
    }
    for (k=i=0;i < 21;i2f[i++]=k+1)
            while (QTable[k] == i)          k++;
static const signed char EscCoef[12]={16,-10,1,51,14,89,23,35,64,26,-42,43};
    for (k=0;k < 64;k++) {
        for (s=i=0;i < 6;i++)               s += EscCoef[2*i+((k >> i) & 1)];
        s=128*CLAMP(s,32,256-32);
        for (i=0;i < 21;i++)                BinSumm[i][k]=BIN_SCALE-s/i2f[i];
    }
    for (i=0;i < 23;i++)
            for (k=0;k < 32;k++)            SEE2Cont[i][k].init(8*i+5);
}
void PPM_CONTEXT::rescale()
{
    UINT f0, sf, f, EscFreq, a=(OrderFall != 0), i=NumStats;
    STATE tmp, *p1, *p, *Stats=getStats();  Flags &= 0x14;
    for (p=FoundState;p != Stats;p--)       SWAP(p[0],p[-1]);
    f0=f=p->f();                            sf=SummFreq;
    EscFreq=SummFreq-f;                     SummFreq=f=(f+a) >> 1;
    p->Freq=(p->Freq & 0x80)+f;
    do {
        f=(++p)->f();                       EscFreq -= f;
        SummFreq += (f=(f+a) >> 1);         p->Freq=(p->Freq & 0x80)+f;
        if ( f )                            Flags |= 0x08*(p->Symbol >= 0x40);
        if (f > p[-1].f()) {
            StateCpy(tmp,*(p1=p));
            do { StateCpy(p1[0],p1[-1]); } while (f > (--p1)[-1].f());
            StateCpy(*p1,tmp);
        }
    } while ( --i );
    if (p->f() == 0) {
        do { i++; } while ((--p)->f() == 0);
        EscFreq += i;                       a=(NumStats+2) >> 1;
        if ((NumStats -= i) == 0) {
            StateCpy(tmp,*Stats);           Flags &= 0x18;
            f=(2*tmp.f()+EscFreq-1)/EscFreq;
            if (f > MAX_FREQ/3)             f=MAX_FREQ/3;
            tmp.Freq=(tmp.Freq & 0x80)+f;
            FreeUnits(wStats,a);            StateCpy(oneState(),tmp);
            FoundState=&oneState();         return;
        }
        wStats = ShrinkUnits(wStats,a,(NumStats+2) >> 1);
    }
    SummFreq += (EscFreq+1) >> 1;
    if (OrderFall || (Flags & 0x04) == 0) {
        a=(sf -= EscFreq)-f0;
        a=CLAMP((f0*SummFreq-sf*Stats->Freq+a-1)/a,2U,MAX_FREQ/2U-18);
    } else                                  a=2;
    (FoundState=getStats())->Freq += a;     SummFreq += a;
    Flags |= 0x04;
}
inline void AuxCutOff(STATE* p,int Order) {
    if (Order < MaxOrder) {
        PPM_CONTEXT* pc=p->getSucc();       PrefetchData(pc);
        if ( pc->cutOff(Order+1) )          return;
    }
    p->wSuccessor=0;                        p->Freq &= ~0x80;
}
BOOL _STDCALL PPM_CONTEXT::cutOff(int Order)
{
    int f, i, tmp, Scale;
    STATE* Stats, * p;
    if ( !NumStats ) {
        if ( (p=&oneState())->hasSucc() ) {
            AuxCutOff(p,Order);             return TRUE;
        }
REMOVE: FreeContext(this);                  return FALSE;
    }
    wStats=MoveUnits(wStats,tmp=(NumStats+2) >> 1);
    for (p=(Stats=getStats())+(i=NumStats);p >= Stats;p--)
            if ( p->hasSucc() )             AuxCutOff(p,Order);
            else { p->wSuccessor=0;         SWAP(*p,Stats[i--]); }
    if (i != NumStats && Order) {
        NumStats=i;
        if (i < 0) { FreeUnits(wStats,tmp); goto REMOVE; }
        else if ( !i ) {
            i=wStats;                       p=getStats();
            Flags=(Flags & 0x10)+0x08*(p->Symbol >= 0x40);
            f=p->f();
            p->Freq=(p->Freq & 0x80)+1+(2*(f-1))/(SummFreq-f);
            StateCpy(oneState(),*p);        FreeUnits(i,tmp);
        } else {
            wStats=ShrinkUnits(wStats,tmp,(i+2) >> 1);
            f=(p=getStats())->f();          Scale=(SummFreq > 16*i);
            Flags=(Flags & (0x10+0x04*Scale))+0x08*(p->Symbol >= 0x40);
            tmp=SummFreq-f;                 SummFreq=f=(f+Scale) >> Scale;
            p->Freq=(p->Freq & 0x80)+f;
            do {
                f=(++p)->f();               tmp -= f;
                SummFreq += (f=(f+Scale) >> Scale);
                p->Freq=(p->Freq & 0x80)+f; Flags |= 0x08*(p->Symbol >= 0x40);
            } while ( --i );
            SummFreq += (tmp+Scale) >> Scale;
        }
    }
    return TRUE;
}
static void RestoreModelRare()
{
    OrderFall=MaxOrder;                     NCutOffs++;
    while ( MaxContext->wSuffix )           MaxContext=MaxContext->suff();
    MaxContext->cutOff(0);                  TextIndx=1;
}
static UINT _FASTCALL CreateSuccessors(BOOL Skip,STATE* p,PPM_CONTEXT* pc)
{
    PPM_CONTEXT ct;
    STATE* ps[MAX_O], ** pps=ps;
    UINT cf, s0, TIndx=FoundState->wSuccessor;
    BYTE tmp, sym=FoundState->Symbol;
    if ( !Skip ) {
        *pps++ = FoundState;
        if ( !pc->wSuffix )                 goto NO_LOOP;
    }
    if ( p ) { pc=pc->suff();               goto LOOP_ENTRY; }
    do {
        pc=pc->suff();
        if ( pc->NumStats ) {
            if ((p=pc->getStats())->Symbol != sym)
                    do { tmp=p[1].Symbol;   p++; } while (tmp != sym);
            tmp=(p->f() < MAX_FREQ-1);
            p->Freq += tmp;                 pc->SummFreq += tmp;
        } else {
            p=&(pc->oneState());
            p->Freq += (!pc->suff()->NumStats & (p->f() < 11));
        }
LOOP_ENTRY:
        if ( p->hasSucc() ) {
            pc=p->getSucc();                break;
        }
        *pps++ = p;
    } while ( pc->wSuffix );
NO_LOOP:
    if (pps == ps)                          return Cont2Pos(pc);
    ct.NumStats=0;                          ct.Flags=0x10*(sym >= 0x40);
    ct.oneState().Symbol=sym=TextBuf[TIndx];
    ct.oneState().wSuccessor=TIndx+1;
    ct.Flags |= 0x08*(sym >= 0x40);
    if ( pc->NumStats ) {
        if ((p=pc->getStats())->Symbol != sym)
                do { tmp=p[1].Symbol;       p++; } while (tmp != sym);
        s0=pc->SummFreq-pc->NumStats-(cf=p->f()-1);
        cf=1+((2*cf <= s0)?(12*cf > s0):((cf+2*s0)/s0));
        ct.oneState().Freq=(cf < 7)?(cf):(7);
    } else
            ct.oneState().Freq=pc->oneState().f();
    s0=Cont2Pos(pc);
    do {
        if ((cf=AllocContext()) == 0)       return 0;
        pc=GetCont(cf);
        ((DWORD*)pc)[0]=((DWORD*)&ct)[0];   ((DWORD*)pc)[1]=((DWORD*)&ct)[1];
        pc->wSuffix=s0;                     (*--pps)->wSuccessor=s0=cf;
        (*pps)->Freq |= 0x80;
    } while (pps != ps);
    return cf;
}
inline UINT ReduceOrder(STATE* p,PPM_CONTEXT* pc)
{
    STATE* p1;
    PPM_CONTEXT* pc1=pc;
    UINT i;
    BYTE tmp, sym=FoundState->Symbol;
    FoundState->wSuccessor=TextIndx;        OrderFall++;
    if ( p ) { pc=pc->suff();               goto LOOP_ENTRY; }
    for ( ; ; ) {
        if ( !pc->wSuffix )                 return Cont2Pos(pc);
        pc=pc->suff();
        if ( pc->NumStats ) {
            if ((p=pc->getStats())->Symbol != sym)
                    do { tmp=p[1].Symbol;   p++; } while (tmp != sym);
            tmp=2*(p->f() < MAX_FREQ-3);
            p->Freq += tmp;                 pc->SummFreq += tmp;
        } else { p=&(pc->oneState());       p->Freq += (p->f() < 11); }
LOOP_ENTRY:
        if ( p->wSuccessor )                break;
        p->wSuccessor=TextIndx;             OrderFall++;
    }
    i=p->wSuccessor;
    if ( !p->hasSucc() ) {
        p1=FoundState;                      FoundState=p;
        i=CreateSuccessors(FALSE,NULL,pc);  FoundState=p1;
    }
    if (OrderFall == 1 && pc1 == MaxContext) {
        FoundState->wSuccessor=i;           FoundState->Freq |= p->Freq & 0x80;
        TextIndx--;
    }
    return i;
}
// Tabulated escapes for exponential symbol distribution
static const BYTE ExpEscape[16]={51,43,18,12,11,9,8,7,6,5,4,3,3,2,2,2};
static void _FASTCALL UpdateModel(PPM_CONTEXT* MinContext)
{
    STATE* p=NULL;
    PPM_CONTEXT* pc;
    UINT ns1, ns, cf, sf, s0, FSuccessor, FFreq, TempSucc, SuccFlag=0;
    BYTE Flag, sym, FSymbol=FoundState->Symbol;
    FSuccessor=FoundState->wSuccessor;      FFreq=FoundState->f();
    if (OrderFall != MaxOrder) {
        if ( (pc=MinContext->suff())->NumStats ) {
            pc->wStats=MoveUnits(pc->wStats,(pc->NumStats+2) >> 1);
            if ((p=pc->getStats())->Symbol != FSymbol) {
                do { sym=p[1].Symbol;       p++; } while (sym != FSymbol);
                if (p[0].f() >= p[-1].f()) {
                    SWAP(p[0],p[-1]);       p--;
                }
            }
            if (p->f() < MAX_FREQ-2) {
                cf=1+(FFreq < 4*8);
                p->Freq += cf;              pc->SummFreq += cf;
            }
        } else { p=&(pc->oneState());       p->Freq += (p->f() < 11); }
    }
    pc=MaxContext;
    if (!OrderFall && FSuccessor) {
        FoundState->wSuccessor=CreateSuccessors(TRUE,p,MinContext);
        if ( !FoundState->wSuccessor )      goto RESTART_MODEL;
        FoundState->Freq |= 0x80;           MaxContext=FoundState->getSucc();
        return;
    }
    TextBuf[TextIndx++] = FSymbol;          TempSucc=TextIndx;
    if (TextIndx == TEXT_BUF_SIZE)          goto RESTART_MODEL;
    if ( FSuccessor ) {
        if ( !FoundState->hasSucc() )
                FSuccessor=CreateSuccessors(FALSE,p,MinContext);
        else                                PrefetchData(GetCont(FSuccessor));
    } else
                FSuccessor=ReduceOrder(p,MinContext);
    if ( !FSuccessor )                      goto RESTART_MODEL;
    if (!--OrderFall && pc != MinContext) {
        TempSucc=FSuccessor;                SuccFlag=0x80;
        TextIndx--;
    }
    s0=MinContext->SummFreq-FFreq;          ns=MinContext->NumStats;
    for (Flag=0x08*(FSymbol >= 0x40);pc != MinContext;pc=pc->suff()) {
        if ((ns1=pc->NumStats) != 0) {
            if ((ns1 & 1) != 0) {
                cf=ExpandUnits(pc->wStats,(ns1+1) >> 1);
                if ( !cf )                  goto RESTART_MODEL;
                pc->wStats=cf;
            }
            pc->SummFreq += QTable[ns+4] >> 3;
        } else {
            if ((cf=AllocUnits(1)) == 0)    goto RESTART_MODEL;
            StateCpy(*(p=(STATE*)(SUnits+cf)),pc->oneState());
            pc->wStats=cf;                  sf=p->f();
            p->Freq=(p->Freq & 0x80)+((sf <= MAX_FREQ/3)?(2*sf-1):(MAX_FREQ-15));
            pc->SummFreq=p->f()+(ns > 1)+ExpEscape[QTable[BSumm >> 8]];
        }
        cf=2*FFreq*(pc->SummFreq+4);        sf=s0+pc->SummFreq;
        if (cf <= 6*sf) {
            cf=1+(cf > sf)+(cf > 3*sf);     pc->SummFreq += 4;
        } else
                pc->SummFreq += (cf=4+(cf > 8*sf)+(cf > 10*sf)+(cf > 13*sf));
        p=pc->getStats()+(++pc->NumStats);  p->wSuccessor=TempSucc;
        p->Symbol = FSymbol;                p->Freq = SuccFlag | cf;
        pc->Flags |= Flag;
    }
    MaxContext=GetCont(FSuccessor);         return;
RESTART_MODEL:
    RestoreModelRare();
}
#define GET_MEAN(SUMM,SHIFT) ((SUMM+ROUND) >> SHIFT)
inline void PPM_CONTEXT::encodeBinSymbol(int symbol)
{
    STATE& rs=oneState();                   UINT f=rs.f();
    WORD& bs=BinSumm[QTable[f-1]][NS2BSIndx[suff()->NumStats]+PrevSuccess+
            Flags+((RunLength >> 26) & 0x20)];
    UINT s, tmp=rcBinStart(s=bs,TOT_BITS);  bs -= GET_MEAN(s,PERIOD_BITS);
    if (rs.Symbol == symbol) {
        bs += INTERVAL;                     rcBinCorrect0(tmp);
        FoundState=&rs;                     rs.Freq += (f < 127);
        RunLength++;                        PrevSuccess=1;
    } else {
        rcBinCorrect1(tmp,BIN_SCALE-(BSumm=s));
        CharMask[rs.Symbol]=EscCount;       FoundState=NULL;
        NumMasked=PrevSuccess=0;
    }
}
inline void PPM_CONTEXT::decodeBinSymbol()
{
    STATE& rs=oneState();                   UINT f=rs.f();
    WORD& bs=BinSumm[QTable[f-1]][NS2BSIndx[suff()->NumStats]+PrevSuccess+
            Flags+((RunLength >> 26) & 0x20)];
    UINT s, tmp=rcBinStart(s=bs,TOT_BITS);  bs -= GET_MEAN(s,PERIOD_BITS);
    if ( !rcBinDecode(tmp) ) {
        bs += INTERVAL;                     rcBinCorrect0(tmp);
        FoundState=&rs;                     rs.Freq += (f < 127);
        RunLength++;                        PrevSuccess=1;
    } else {
        rcBinCorrect1(tmp,BIN_SCALE-(BSumm=s));
        CharMask[rs.Symbol]=EscCount;       FoundState=NULL;
        NumMasked=PrevSuccess=0;
    }
}
inline void PPM_CONTEXT::update1(STATE* p)
{
    UINT f;
    (FoundState=p)->Freq += 4;              SummFreq += 4;
    if ((f=p[0].f()) > p[-1].f()) {
        SWAP(p[0],p[-1]);                   FoundState=--p;
        if (f >= MAX_FREQ)                  rescale();
    }
}
inline void PPM_CONTEXT::encodeSymbol1(int symbol)
{
    STATE* p=getStats();
    UINT i=p->Symbol, LoCnt=p->f();         Range.scale=SummFreq;
    if (i == symbol) {
        PrevSuccess=(2*(Range.high=LoCnt) > Range.scale);
        (FoundState=p)->Freq += 4;          SummFreq += 4;
        if (LoCnt >= MAX_FREQ-4)            rescale();
        Range.low=0;                        return;
    }
    i=NumStats;                             PrevSuccess=0;
    while ((++p)->Symbol != symbol) {
        LoCnt += p->f();
        if (--i == 0) {
            PrefetchData(suff());
            Range.low=LoCnt;                CharMask[p->Symbol]=EscCount;
            i=NumMasked=NumStats;           FoundState=NULL;
            do { CharMask[(--p)->Symbol]=EscCount; } while ( --i );
            Range.high=Range.scale;         return;
        }
    }
    Range.high=(Range.low=LoCnt)+p->f();    update1(p);
}
inline void PPM_CONTEXT::decodeSymbol1()
{
    STATE* p=getStats();
    UINT i, count, HiCnt=p->f();            Range.scale=SummFreq;
    if ((count=rcGetCurrentCount()) < HiCnt) {
        PrevSuccess=(2*(Range.high=HiCnt) > Range.scale);
        (FoundState=p)->Freq += 4;          SummFreq += 4;
        if (HiCnt >= MAX_FREQ-4)            rescale();
        Range.low=0;                        return;
    }
    i=NumStats;                             PrevSuccess=0;
    while ((HiCnt += (++p)->f()) <= count)
        if (--i == 0) {
            PrefetchData(suff());
            Range.low=HiCnt;                CharMask[p->Symbol]=EscCount;
            i=NumMasked=NumStats;           FoundState=NULL;
            do { CharMask[(--p)->Symbol]=EscCount; } while ( --i );
            Range.high=Range.scale;         return;
        }
    Range.low=(Range.high=HiCnt)-p->f();    update1(p);
}
inline void PPM_CONTEXT::update2(STATE* p)
{
    (FoundState=p)->Freq += 4;              SummFreq += 4;
    if (p->f() >= MAX_FREQ)                 rescale();
    EscCount++;                             RunLength=InitRL;
}
inline SEE2_CONTEXT* PPM_CONTEXT::makeEscFreq2()
{
    SEE2_CONTEXT* psee2c;                   PrefetchData(getStats());
    if (NumStats != 0xFF) {
        psee2c=SEE2Cont[QTable[NumStats+3]-4]+(SummFreq > 10*(NumStats+1))+
                2*(2*NumStats < suff()->NumStats+NumMasked)+Flags;
        Range.scale=psee2c->getMean();
    } else { psee2c=&DummySEE2Cont;         Range.scale=1; }
    PrefetchData(getStats()+NumStats);      return psee2c;
}
inline void PPM_CONTEXT::encodeSymbol2(int symbol)
{
    SEE2_CONTEXT* psee2c=makeEscFreq2();
    UINT Sym, LoCnt=0, i=NumStats-NumMasked;
    STATE* p1, * p=getStats()-1;
    do {
        do { Sym=p[1].Symbol;   p++; } while (CharMask[Sym] == EscCount);
        CharMask[Sym]=EscCount;
        if (Sym == symbol)                  goto SYMBOL_FOUND;
        LoCnt += p->f();
    } while ( --i );
    Range.high=(Range.scale += (Range.low=LoCnt));
    psee2c->Summ += Range.scale;            NumMasked = NumStats;
    return;
SYMBOL_FOUND:
    Range.low=LoCnt;                        Range.high=(LoCnt += p->f());
    for (p1=p; --i ; ) {
        do { Sym=p1[1].Symbol;  p1++; } while (CharMask[Sym] == EscCount);
        LoCnt += p1->f();
    }
    Range.scale += LoCnt;
    psee2c->update();                       update2(p);
}
inline void PPM_CONTEXT::decodeSymbol2()
{
    SEE2_CONTEXT* psee2c=makeEscFreq2();
    UINT Sym, count, HiCnt=0, i=NumStats-NumMasked;
    STATE* ps[256], ** pps=ps, * p=getStats()-1;
    do {
        do { Sym=p[1].Symbol;   p++; } while (CharMask[Sym] == EscCount);
        HiCnt += p->f();                    *pps++ = p;
    } while ( --i );
    Range.scale += HiCnt;                   count=rcGetCurrentCount();
    p=*(pps=ps);
    if (count < HiCnt) {
        HiCnt=0;
        while ((HiCnt += p->f()) <= count)  p=*++pps;
        Range.low = (Range.high=HiCnt)-p->f();
        psee2c->update();                   update2(p);
    } else {
        Range.low=HiCnt;                    Range.high=Range.scale;
        i=NumStats-NumMasked;               NumMasked = NumStats;
        do { CharMask[(*pps)->Symbol]=EscCount; pps++; } while ( --i );
        psee2c->Summ += Range.scale;
    }
}
inline void ClearMask(_PPMD_FILE* EncodedFile,_PPMD_FILE* DecodedFile)
{
    EscCount=1;                             memset(CharMask,0,sizeof(CharMask));
    if (++PrintCount == 0)                  PrintInfo(DecodedFile,EncodedFile);
}
void _STDCALL EncodeFile(_PPMD_FILE* EncodedFile,_PPMD_FILE* DecodedFile,int MaxOrder)
{
    rcInitEncoder();                        StartModelRare(MaxOrder);
    for (PPM_CONTEXT* MinContext=MaxContext; ; ) {
        int c = _PPMD_E_GETC(DecodedFile);
        if ( MinContext->NumStats ) {
            MinContext->encodeSymbol1(c);   rcEncodeSymbol();
        } else                              MinContext->encodeBinSymbol(c);
        while ( !FoundState ) {
            RC_ENC_NORMALIZE(EncodedFile);
            do {
                if ( !MinContext->wSuffix ) goto STOP_ENCODING;
                OrderFall++;                MinContext=MinContext->suff();
            } while (MinContext->NumStats == NumMasked);
            MinContext->encodeSymbol2(c);   rcEncodeSymbol();
        }
        if (!OrderFall && FoundState->hasSucc())
                PrefetchData(MaxContext=FoundState->getSucc());
        else {
            UpdateModel(MinContext);
            if (EscCount == 0)              ClearMask(EncodedFile,DecodedFile);
        }
        RC_ENC_NORMALIZE(EncodedFile);      MinContext=MaxContext;
    }
STOP_ENCODING:
    rcFlushEncoder(EncodedFile);            PrintInfo(DecodedFile,EncodedFile);
}
void _STDCALL DecodeFile(_PPMD_FILE* DecodedFile,_PPMD_FILE* EncodedFile,int MaxOrder)
{
    rcInitDecoder(EncodedFile);             StartModelRare(MaxOrder);
    for (PPM_CONTEXT* MinContext=MaxContext; ; ) {
        if ( MinContext->NumStats ) {
            MinContext->decodeSymbol1();    rcRemoveSubrange();
        } else                              MinContext->decodeBinSymbol();
        while ( !FoundState ) {
            RC_DEC_NORMALIZE(EncodedFile);
            do {
                if ( !MinContext->wSuffix ) goto STOP_DECODING;
                OrderFall++;                MinContext=MinContext->suff();
            } while (MinContext->NumStats == NumMasked);
            MinContext->decodeSymbol2();    rcRemoveSubrange();
        }
        _PPMD_D_PUTC(FoundState->Symbol,DecodedFile);
        if (!OrderFall && FoundState->hasSucc())
                PrefetchData(MaxContext=FoundState->getSucc());
        else {
            UpdateModel(MinContext);
            if (EscCount == 0)              ClearMask(EncodedFile,DecodedFile);
        }
        RC_DEC_NORMALIZE(EncodedFile);      MinContext=MaxContext;
    }
STOP_DECODING:
    PrintInfo(DecodedFile,EncodedFile);
}
