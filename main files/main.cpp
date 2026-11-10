// ======================= TIME-TRAVEL DEBUGGER - SERVER TEMPLATE =======================

// Pipeline this file implements, top to bottom:
//   0. Receive  -- stream the client's .trace bytes straight to source.bin on disk
//   1. Pass 0X0   -- validity check (FUNC/FUNC_END matching)
//   2. Pass 0X1   -- resolve(): copy EVERY source line into resolve.bin as [offset][size][string], then patch CALL targets.
//   3. Pass 0X2   -- execute resolve.bin: tokenize ONE line at a time, update the call stack, take a snapshot -> Timeline
//   4. Pass 0X3   -- serialize Timeline -> session.tdbg(header + snapshot records + dense index)

/*
func foo b
set a 10
add b a
func_end
func main
set k 10
call foo k
func_end
*/

#include <iostream>
#include <string>
#include <cstdint>
#include <fstream>
#ifdef _WIN32
    #include <ws2tcpip.h>
    #include <winsock2.h>
#else
    #include <unistd.h>
    #include <sys/socket.h>
#endif
#include <cstdio>
using namespace std;


#include "constants.h"
#include "stack.h"
#include "timeline.h"
#include "common.h"
#include "validate.h"
#include "resolve.h"
#include "execute.h"
#include "serialize.h"



// main section
int32_t main()
{
    try{
        
        cout<<"compile starting"<<endl;
        if (!validateProgram("extra files/source.bin"))
        {
            throw "program has nested function or a end function line is present without the start line or no endline for a function";
        }
        cout<<"hi"<<endl;
        int64_t mainOffset = resolveProgram("extra files/source.bin", "extra files/resolve.bin");

        Timeline timeline;
        executeProgram("extra files/resolve.bin", mainOffset, timeline);

        writeTdbg(timeline, "extra files/session.tdbg");
    }
    catch (const char* error){
        cerr << "Error: " << error << endl;
        return 1;
    }
    return 0;
}