/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * Jit control
 */
#ifndef _DALVIK_INTERP_JIT
#define _DALVIK_INTERP_JIT

#include "InterpDefs.h"
#include "mterp/common/jit-config.h"

#define JIT_MAX_TRACE_LEN 100

#if defined (WITH_SELF_VERIFICATION)

#define REG_SPACE 256                /* default size of shadow space */
#define HEAP_SPACE JIT_MAX_TRACE_LEN /* default size of heap space */

typedef struct ShadowHeap {
    int addr;
    int data;
} ShadowHeap;

typedef struct InstructionTrace {
    int addr;
    DecodedInstruction decInsn;
} InstructionTrace;

typedef struct ShadowSpace {
    const u2* startPC;          /* starting pc of jitted region */
    const void* fp;             /* starting fp of jitted region */
    void* glue;                 /* starting glue of jitted region */
    SelfVerificationState jitExitState;  /* exit point for JIT'ed code */
    SelfVerificationState selfVerificationState;  /* current SV running state */
    const u2* endPC;            /* ending pc of jitted region */
    void* shadowFP;       /* pointer to fp in shadow space */
    InterpState interpState;    /* copy of interpState */
    int* registerSpace;         /* copy of register state */
    int registerSpaceSize;      /* current size of register space */
    ShadowHeap heapSpace[HEAP_SPACE]; /* copy of heap space */
    ShadowHeap* heapSpaceTail;        /* tail pointer to heapSpace */
    const void* endShadowFP;    /* ending fp in shadow space */
    InstructionTrace trace[JIT_MAX_TRACE_LEN]; /* opcode trace for debugging */
    int traceLength;            /* counter for current trace length */
    const Method* method;       /* starting method of jitted region */
} ShadowSpace;

/*
 * Self verification functions.
 */
void* dvmSelfVerificationShadowSpaceAlloc(Thread* self);
void dvmSelfVerificationShadowSpaceFree(Thread* self);
void* dvmSelfVerificationSaveState(const u2* pc, const void* fp,
                                   InterpState* interpState,
                                   int targetTrace);
void* dvmSelfVerificationRestoreState(const u2* pc, const void* fp,
                                      SelfVerificationState exitPoint);
#endif

/*
 * JitTable hash function.
 */

static inline u4 dvmJitHashMask( const u2* p, u4 mask ) {
    return ((((u4)p>>12)^(u4)p)>>1) & (mask);
}

static inline u4 dvmJitHash( const u2* p ) {
    return dvmJitHashMask( p, gDvmJit.jitTableMask );
}

/*
 * The width of the chain field in JitEntryInfo sets the upper
 * bound on the number of translations.  Be careful if changing
 * the size of JitEntry struct - the Dalvik PC to JitEntry
 * hash functions have built-in knowledge of the size.
 */
#define JIT_ENTRY_CHAIN_WIDTH 2
#define JIT_MAX_ENTRIES (1 << (JIT_ENTRY_CHAIN_WIDTH * 8))

/*
 * The trace profiling counters are allocated in blocks and individual
 * counters must not move so long as any referencing trace exists.
 */
#define JIT_PROF_BLOCK_ENTRIES 1024
#define JIT_PROF_BLOCK_BUCKETS (JIT_MAX_ENTRIES / JIT_PROF_BLOCK_ENTRIES)

typedef s4 JitTraceCounter_t;

typedef struct JitTraceProfCounters {
    unsigned int           next;
    JitTraceCounter_t      *buckets[JIT_PROF_BLOCK_BUCKETS];
} JitTraceProfCounters;

/*
 * Entries in the JIT's address lookup hash table.
 * Fields which may be updated by multiple threads packed into a
 * single 32-bit word to allow use of atomic update.
 */

typedef struct JitEntryInfo {
    unsigned int           traceConstruction:1;   /* build underway? */
    unsigned int           isMethodEntry:1;
    unsigned int           inlineCandidate:1;
    unsigned int           profileEnabled:1;
    JitInstructionSetType  instructionSet:4;
    unsigned int           profileOffset:8;
    u2                     chain;                 /* Index of next in chain */
} JitEntryInfo;

typedef union JitEntryInfoUnion {
    JitEntryInfo info;
    volatile int infoWord;
} JitEntryInfoUnion;

typedef struct JitEntry {
    JitEntryInfoUnion   u;
    const u2*           dPC;            /* Dalvik code address */
    void*               codeAddress;    /* Code address of native translation */
} JitEntry;

int dvmCheckJit(const u2* pc, Thread* self, InterpState* interpState,
                const ClassObject *callsiteClass, const Method* curMethod);
void* dvmJitGetTraceAddr(const u2* dPC);
void* dvmJitGetMethodAddr(const u2* dPC);
bool dvmJitCheckTraceRequest(Thread* self, InterpState* interpState);
void dvmJitStopTranslationRequests(void);
void dvmJitStats(void);
bool dvmJitResizeJitTable(unsigned int size);
void dvmJitResetTable(void);
struct JitEntry *dvmFindJitEntry(const u2* pc);
s8 dvmJitd2l(double d);
s8 dvmJitf2l(float f);
void dvmJitSetCodeAddr(const u2* dPC, void *nPC, JitInstructionSetType set,
                       bool isMethodEntry, int profilePrefixSize);
void dvmJitAbortTraceSelect(InterpState* interpState);
JitTraceCounter_t *dvmJitNextTraceCounter(void);
void dvmJitTraceProfilingOff(void);
void dvmJitTraceProfilingOn(void);
void dvmJitChangeProfileMode(TraceProfilingModes newState);

#endif /*_DALVIK_INTERP_JIT*/
