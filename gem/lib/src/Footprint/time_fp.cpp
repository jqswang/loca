/*
  This tool is used to measure the time reuse distance.
  It outputs the time reuse distance histogram.
  -std=c++14 is required to compile this file
*/

#include <iostream>
#include <fstream>
#include <stdlib.h>

#include "pin.H"
#include "portability.H"

using namespace std;
KNOB<string> KnobResultFile(KNOB_MODE_WRITEONCE, "pintool",
			    "o", "memory_trace.out", "specify result file name");

#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <unordered_map>

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */
std::ofstream ResultFile;

#define CACHE_LINE_BITS 2 // 4 bytes
#define GET_CACHE_LINE(addr) ((uint64_t)addr >> CACHE_LINE_BITS)

uint64_t g_time_counter = 0;
unordered_map<uint64_t, uint64_t> g_last_use_time;
unordered_map<uint32_t, uint32_t> g_reuse_histo;


struct timeval start;//start time of profiling
struct timeval finish;//stop time of profiling


/* ===================================================================== */

INT32 Usage() {
    cerr << "This tool prints out the number of dynamic instructions executed to stderr.\n";
    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;
    return -1;
}

/* ===================================================================== */



/*
 * Callback Routines
 */


VOID RecordMem(VOID * ip, VOID * addr)
{
  g_time_counter ++;
  uint64_t target = GET_CACHE_LINE(addr);
  auto it = g_last_use_time.find(target);
  if( it == g_last_use_time.end()){
    g_last_use_time.insert({target, g_time_counter});
    return;
  }

  uint32_t rd = g_time_counter - it->second;
  it->second = g_time_counter;

  auto it2 = g_reuse_histo.find(rd);
  if ( it2 == g_reuse_histo.end()){
    g_reuse_histo.insert({rd, 1});
  }
  else {
    it2->second ++;
  }
}


VOID Instruction(INS ins, VOID *v) {
    // Instruments loads using a predicated call, i.e.
    // the call happens iff the load will be actually executed.
    // The IA-64 architecture has explicitly predicated instructions. 
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP 
    // prefixed instructions appear as predicated instructions in Pin.
    if (INS_IsMemoryRead(ins)) {
      INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMem,
			   IARG_INST_PTR, IARG_MEMORYREAD_EA, 
			   IARG_END);
    }
    if (INS_HasMemoryRead2(ins)) {
      INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMem,
			   IARG_INST_PTR, IARG_MEMORYREAD2_EA, 
			   IARG_END);
    }
    if (INS_IsMemoryWrite(ins))	{
      INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMem,
			   IARG_INST_PTR, IARG_MEMORYWRITE_EA, 
			   IARG_END);
    }
}



/* ===================================================================== */

VOID Fini(INT32 code, VOID *v){
  
  gettimeofday(&finish, 0);
  // time = (finish.tv_sec - start.tv_sec);

  ResultFile.open(KnobResultFile.Value().c_str());
  ResultFile << "#ACCESSES: " << g_time_counter << std::endl;
  ResultFile << "#ELEMENTS: " << g_last_use_time.size() << std::endl; 
  ResultFile << "#  [time reuse distance]\t[occurrence]" << std::endl;
  for(auto const& x : g_reuse_histo){
    ResultFile << x.first << "\t" << x.second << std::endl;

  }
  ResultFile.close();  
  
}

/* ===================================================================== */

int main(int argc, char *argv[])
{
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    gettimeofday(&start, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
