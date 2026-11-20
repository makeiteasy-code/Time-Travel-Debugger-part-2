// ======================= TIME-TRAVEL DEBUGGER - CLIENT TEMPLATE =======================

// The client never talks to the server directly. It only talks to the load balancer.
// The load balancer wakes the server up if it is down, so the client does not care.
// The first "start" may just take a bit longer.
//
// The network is used ONLY by "start". After the .tdbg file is downloaded,
// next / prv / jmp read that file on the local disk. They never contact the load balancer.
//
// How the pieces fit together:
//   main loop -> readCommand -> runCommand
//   start -> cmdStart()  -> requestDebugFile() (Part A) -> openTdbg() (Part B) -> shows step 1
//   next, prv, jmp -> moveBy() (Part D) -> readSnapshotAt() (Part B) -> printSnapshot() (Part E)

#include <iostream>
#include <string>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <vector>
#include <sys/socket.h>
#include "helper.h"
#include <arpa/inet.h>
#include <netinet/in.h>
using namespace std;

// ---- Constants ----
const char *PROGRAM_NAME = "ttdb";
const char *PROMPT = "ttdb> ";
const int IO_BUFFER_SIZE = 64 * 1024;     // fixed buffer for streaming files to and from the socket
const char *TDBG_FILE = "ttdb_last.tdbg"; // where the downloaded .tdbg is saved
const int MAX_JUMP = 1000000000;          // biggest n accepted by jmp
string LB_HOST = "127.0.0.0";
int LB_PORT = 5000;

// These two must be the same as in Phase 1

const int MAX_VARS_PER_FRAME = 16;
const int MAX_STACK_DEPTH = 64;


// ---- Structs (same as Phase 1, so the file can be read back exactly as it was written) ----
struct Variable
{
    string name;
    int32_t value;
};
struct Frame
{
    string func_name;
    int32_t argc;
    Variable argv[MAX_VARS_PER_FRAME];
    int64_t returnLine;
    Variable locals[MAX_VARS_PER_FRAME];
    int32_t localCount;
};
struct Snapshot
{
    Frame callStack[MAX_STACK_DEPTH];
    int32_t stackDepth;
};
struct TTDBHeader 
{
    char magic[4]; // "TTDB"
    int32_t version;
    int32_t stepCount;
    int64_t indexOffset;
};

// ---- Structs only the client needs ----
struct TdbgReader
{
    FILE *file;
    TTDBHeader header;
    vector<int64_t> index;
};
struct DebugSession
{
    int active_flag;     // false until a "start" has worked
    TdbgReader reader;   // the .tdbg file stays open for the whole session
    int64_t currentStep; 
};

// =============================================================================================
// PART A: TALKING TO THE LOAD BALANCER
// =============================================================================================

bool recvAll(int sock, char *buff, size_t n)
{
    size_t receivedTotal=0;
    while(n>receivedTotal){
        int received = (int)recv(sock,receivedTotal+buff,n-receivedTotal,0);
        if(0>=received)return 0;
        receivedTotal+=received;
    }
    return 1;


    // recv() can return fewer bytes than asked for, so keep calling it until exactly n bytes are read.
    // Return false if the connection closes or fails before n bytes arrive.
}
bool sendAll(int sock, const char *buff, size_t n)
{
    size_t sentTotal=0;
    while(n>sentTotal){
        int sent = (int)send(sock,sentTotal+buff,n-sentTotal,MSG_NOSIGNAL);
        if(0>=sent)return 0;
        sentTotal+=sent;
    }
    return 1;

    // send() can send fewer bytes than asked for, so keep calling it until all n bytes are sent.
    // Use the flag MSG_NOSIGNAL. Return false if the connection fails.
}

int connectToHost(const char *host, int port)
{
    int sock=socket(AF_INET,SOCK_STREAM,0);
    if (sock == -1){
        return -1;
    }
    sockaddr_in serverAddress={};
    serverAddress.sin_family=AF_INET;
    serverAddress.sin_port=htons(port);

    int result =inet_pton(AF_INET,host,&serverAddress.sin_addr);
    if (result <=0){
        close(sock);
        return -1;
    }
    result = connect(sock,(sockaddr*)&serverAddress,sizeof(serverAddress));

    if (result == -1){
        close(sock);
        return -1;
    }
    return sock;

    // Create a TCP socket and connect it to host:port.
    // Return the socket, or -1 if the connection fails.
}
bool uploadSource(int sock,const char *sourcePath)
{
    FILE *file= fopen(sourcePath,"rb");
    if (file==nullptr)return 0;
    char buffer[IO_BUFFER_SIZE];
    size_t bytesRead= fread(buffer,1,IO_BUFFER_SIZE,file);
    uint32_t checksum =0;

    while (bytesRead>0){
        for (size_t i=0;i < bytesRead;i++){
            checksum += static_cast<unsigned char>(buffer[i]);
        }
        if (!sendAll(sock, buffer, bytesRead)){
            fclose(file);
            return false;
        }
        bytesRead = fread(buffer,1,IO_BUFFER_SIZE,file);

        

    }
    if (ferror(file)){
        fclose(file);
        return 0;
    }
    fclose(file);

    if (!sendAll(sock,(const char*)&checksum,sizeof(checksum))) return 0;
    if (shutdown(sock,SHUT_WR) ==-1)return 0;
    
    return 1;
    // have to do checksum and then send
    // When the file is finished, call shutdown(sock, SHUT_WR). This is how the server knows the string is complete.
    // Return false if the file cannot be opened or any send fails.
}
bool receiveResponse(int sock, const char *outTdbgPath, string &outError)
{
    char status;
    if(!recvAll(sock,&status,1)){
        outError = "failed to receive response status";
        return 0;
    }
    if(status != 0 && status != 1){
        outError = "invalid response status";
        return 0;
    }
    FILE *file= fopen(outTdbgPath,"wb");
    if(file==nullptr){
        outError="could not create output file";
        return 0;
    }
    uint32_t checksum=0;
    checksum +=static_cast<unsigned char>(status);
    char buffer[IO_BUFFER_SIZE];
    char lastBytes[4];
    int countlastBytes=0;
    while(1){
        int bytesReceived = recv(sock,buffer,IO_BUFFER_SIZE,0);
        if(bytesReceived < 0){
            fclose(file);
            remove(outTdbgPath);
            outError = "failed to receive response data";
            return false;
        
        }

        if(bytesReceived == 0)break;
        for (int i = 0;i < bytesReceived;i++)
        {
            char currentByte =buffer[i];
            if(countlastBytes<4){
                lastBytes[countlastBytes]=currentByte;
                countlastBytes++;
            }
            else{
                char payloadByte =lastBytes[0];
                checksum += static_cast<unsigned char>(payloadByte);
                if(status==0){
                    if(fwrite(&payloadByte,1,1,file)!=1){
                        fclose(file);
                        remove(outTdbgPath);
                        outError = "failed to write output file";
                        return 0;
                    }
                }
                else{
                    outError += payloadByte;
                }
                lastBytes[0]=lastBytes[1];
                lastBytes[1]=lastBytes[2];
                lastBytes[2]=lastBytes[3];
                lastBytes[3]=currentByte;
            }
        }
    }
    if(countlastBytes!=4){
        fclose(file);
        remove(outTdbgPath);
        outError = "response is incomplete";
        return 0;
    }
    uint32_t receivedChecksum;
    memcpy(&receivedChecksum, lastBytes, sizeof(receivedChecksum));
    if(receivedChecksum!=checksum){
        fclose(file);
        remove(outTdbgPath);
        outError = "checksum mismatch, response corrupted";
        return 0;
    }
    fclose(file);
   if (status == 1){
        remove(outTdbgPath);
        return 0;
    }
    return 1;
    // Reply on the wire: [status][payload][CRC-32]. The CRC covers the status byte too.
    // Feed every received byte (status included) to a TrailerChecker.
    // Status 1: collect the payload into outError. Status 0: write the payload to the file.
    // At the end call verify(). If it fails, delete the file, set outError to
    // "checksum mismatch, response corrupted" and return false.
}

// =============================================================================================
// PART B: READING THE .tdbg FILE
// File layout (written by Phase 1):
//   header | snapshot 0 | snapshot 1 | ... | index (one 8 byte offset per snapshot)
// The index lets us jump straight to any step without reading the steps before it.
// =============================================================================================

bool openTdbg(const char *path, TdbgReader &r){
    r.file=fopen(path,"rb");
    if(r.file==nullptr)return 0;
    if(fread(r.header.magic,1,4,r.file)!=4 || fread(&r.header.version,sizeof(int32_t),1,r.file)!= 1 || fread(&r.header.stepCount,sizeof(int32_t),1,r.file)!=1 || fread(&r.header.indexOffset,sizeof(int64_t),1,r.file)!=1){
        fclose(r.file);
        r.file = nullptr;
        return 0;
    }
    if(memcmp(r.header.magic,"TTDB",4)!=0 || r.header.version!= 1 || r.header.stepCount<=0 || r.header.indexOffset<20){
        fclose(r.file);
        r.file = nullptr;
        return 0;
    }
    if(fseek(r.file,r.header.indexOffset,SEEK_SET)!= 0){
        fclose(r.file);
        r.file = nullptr;
        return 0;
    }

    r.index.resize(r.header.stepCount);
    if(fread(r.index.data(),sizeof(int64_t),r.header.stepCount,r.file)!= r.header.stepCount)
    {
        fclose(r.file);
        r.file = nullptr;
        return 0;
    }

    return 1;

}

void closeTdbg(TdbgReader &r){
    if(r.file != nullptr){
        fclose(r.file);
        r.file = nullptr;
    }
    r.index.clear();
    // Free r.index and close the file.
}
Snapshot *readSnapshotAt(TdbgReader &r, int32_t step){
    if(r.file==nullptr || step<0 || step>=r.header.stepCount)return nullptr;
    if(fseek(r.file,r.index[step],SEEK_SET)!=0)return nullptr;
    Snapshot *snapshot=new Snapshot{};

    if(fread(&snapshot->stackDepth,sizeof(int32_t),1,r.file)!=1 || snapshot->stackDepth<0 || snapshot->stackDepth>MAX_STACK_DEPTH){
        delete snapshot;
        return nullptr;
    }

    for(int i=0;i<snapshot->stackDepth;i++){
        Frame &frame=snapshot->callStack[i];
        int32_t nameSize;
        if (fread(&nameSize,sizeof(int32_t),1,r.file)!= 1||nameSize<0){
            delete snapshot;
            return nullptr;
        }
        frame.func_name.resize(nameSize);
        if(fread(frame.func_name.data(),1,nameSize,r.file)!=nameSize){
            delete snapshot;
            return nullptr;
        }
        if(fread(&frame.argc,sizeof(int32_t),1,r.file)!= 1||frame.argc<0 || frame.argc>MAX_VARS_PER_FRAME){
            delete snapshot;
            return nullptr;
        }

        for (int j=0;j<frame.argc;j++){
            if(fread(&nameSize,sizeof(int32_t),1,r.file)!= 1||nameSize<0){
                delete snapshot;
                return nullptr;
            }
            frame.argv[j].name.resize(nameSize);
            if(fread(frame.argv[j].name.data(),1,nameSize,r.file)!=nameSize){
                delete snapshot;
                return nullptr;
            }
            if(fread(&frame.argv[j].value,sizeof(int32_t),1,r.file)!=1){
                delete snapshot;
                return nullptr;
            }
        }
        if(fread(&frame.returnLine,sizeof(int64_t),1 r.file)!= 1){
            delete snapshot;
            return nullptr;
        }
        if(fread(&frame.localCount,sizeof(int32_t),1,r.file)!=1||frame.localCount<0||frame.localCount>MAX_VARS_PER_FRAME){
            delete snapshot;
            return nullptr;
        }

        for(int j=0;j<frame.localCount;j++){
            if(fread(&nameSize,sizeof(int32_t),1,r.file)!= 1||nameSize<0){
                delete snapshot;
                return nullptr;
            }

            frame.locals[j].name.resize(nameSize);

            if(fread(frame.locals[j].name.data(),1,nameSize,r.file)!=nameSize){
                delete snapshot;
                return nullptr;
            }

            if(fread(&frame.locals[j].value,sizeof(int32_t),1,r.file)!=1){
                delete snapshot;
                return nullptr;
            }
        }
    }
    

    return snapshot;


}

// =============================================================================================
// PART C: THE SESSION IN MEMORY
// There is one DebugSession variable, created in main() and passed to every command by reference.
// =============================================================================================

void initSession(DebugSession &s)
{
    s.active_flag=0;
    s.reader.file=nullptr;
    s.currentStep=0;
}
void closeSession(DebugSession &s)
{
    if(s.active_flag){
        closeTdbg(s.reader);
        s.active_flag=0;
    }
    s.currentStep=0;
    // If active, call closeTdbg(s.reader). Then set active = false.
    // Called when the user types quit and when the input ends.
}

// =============================================================================================
// PART D: THE COMMANDS
// =============================================================================================

void printSnapshot(int32_t step, int32_t totalSteps, const Snapshot &s); // defined in Part E

bool showCurrent(DebugSession &s)
{
    Snapshot *snapshot=readSnapshotAt(s.reader, s.currentStep);
    if(snapshot==nullptr){
        cout <<"damaged .tdbg file"<< endl;
        return 0;
    }
    printSnapshot((s.currentStep + 1),s.reader.header.stepCount,*snapshot);
    delete snapshot;
    return 1;
    // Read the snapshot at s.currentStep, call printSnapshot, delete the snapshot.
    // If readSnapshotAt returns nullptr, print "damaged .tdbg file" and return false.
}
bool moveBy(DebugSession &s, int32_t delta)
{
    if(!s.active_flag){
        cout <<"Debugging session not active."<< endl;
        return 0;
    }
    int64_t newStep=s.currentStep+delta;

    if(newStep<0){
        cout <<"Already at first step."<< endl;
        return 0;
    }
    if(newStep>=s.reader.header.stepCount){
        cout <<"Already at last step."<< endl;
        return 0;
    }
    s.currentStep=newStep;
    return showCurrent(s);
}
bool cmdStart(DebugSession &s, const string &sourcePath)
{
    // 1. requestDebugFile with host and port, saving to TDBG_FILE.
    // 2. If it fails, print the error text and return false. Keep the old session as it was.
    // 3. Open the new file with openTdbg into a NEW local TdbgReader.
    //    If that fails, print "server sent a damaged .tdbg file", keep the old session, return false.
    // 4. Only now close the old reader (if s.active), then put the new reader into s,
    //    set s.currentStep = 0 and s.active = true.
    // 5. Show step 1 with showCurrent(s).
    if(!requestDebugFile(LB_HOST.c_str(),LB_PORT,sourcePath.c_str(),TDBG_FILE, error)){
        cout << "Error: "<<error<<endl;
        return 0;
    }
    TdbgReader reader{};
    if(!openTdbg(TDBG_FILE,reader)){
        cout <<"server sent a damaged .tdbg file"<< endl;
        remove(TDBG_FILE);
        return 0;
    }
    if(s.active_flag){
        closeTdbg(s.reader);
    }

    
    s.reader =reader;
    s.currentStep=0;
    s.active_flag=1;
    return showCurrent(s);
}
bool cmdNext(DebugSession&s) {
    return moveBy(s, 1);
}
bool cmdPrv(DebugSession&s) {
    return moveBy(s,-1);
}
bool cmdJmpNext(DebugSession &s,int32_t n) {
    return moveBy(s,n);
}
bool cmdJmpPrv(DebugSession &s,int32_t n) {
    return moveBy(s,-n);
}

// =============================================================================================
// PART E: PRINTING
// =============================================================================================

void printSnapshot(int32_t step, int32_t totalSteps, const Snapshot &s)
{
    cout << endl<<"======== Step "<<step<<" of "<<totalSteps<<" ========"<< endl;
    cout <<"Stack depth: "<<s.stackDepth<<endl;
    for (int i=0;i<s.stackDepth;i++){
        const Frame &frame=s.callStack[i];
        cout <<"Frame "<< i+1 <<endl;
        cout <<"Function: "<< frame.func_name <<endl;

        cout <<"Arguments:"<<endl;

        if(frame.argc==0)cout <<"  None"<< endl;

        for(int j=0;j<frame.argc;j++){
            cout <<"  "<<frame.argv[j].name<<" = "<<frame.argv[j].value <<endl;
        }
        cout<<"Return line: "<<frame.returnLine<<endl;
        cout<<"Local variables:"<<endl;
        if(frame.localCount==0)cout << "  None" << endl;
        for(int j=0;j<frame.localCount;j++){
            cout<<"  "<<frame.locals[j].name<<" = "<<frame.locals[j].value<<endl;
        }
    }
    cout << "==================================" <<endl<<endl;
}
void printHelp()
{
    cout <<endl<<"Available commands:"<<endl;
    cout<<"  start <source.trace>  Start debugging session"<<endl;
    cout<<"  next                  Move to next step"<<endl;
    cout<<"  prv                   Move to previous step"<<endl;
    cout<<"  jmp next <n>          Move forward by n steps"<<endl;
    cout<<"  jmp prv <n>           Move backward by n steps"<<endl;
    cout<<"  help                  Show this"<<endl;
    cout<<"  quit                  Exit"<<endl;
    cout<<endl;
}


// =============================================================================================
// PART F: THE INFINITE LOOP
// =============================================================================================

bool runCommand(DebugSession &s, const string &line, bool &quitRequested)
{
    // 1. Split the line into words.
    string word;
    vector<string> words;
    bool insideQuotes=0;
    for(int i=0;i<line.length();i++){
        char ch=line[i];
        if(ch=='"')insideQuotes=1;
        else if(ch==' '&&!insideQuotes){
            if (!word.empty()){
                words.push_back(word);
                word.clear();
            }
        }
        else word += ch;
        
    }

    if(!word.empty()) words.push_back(word);
    if (words.empty())return 0;
    // 2. If the first word is "ttdb", drop it.
    if(words[0]==PROGRAM_NAME)words.erase(words.begin());
    if(words.empty()){
        cout <<"unknown command" <<endl;
        printHelp();
        return 0;
    }
    // 3. Match the first word:
    //      start      -> needs a path after it, then cmdStart
    //      next       -> no more words, then cmdNext
    //      prv        -> no more words, then cmdPrv
    //      jmp        -> second word "next" or "prv", third word a positive number ,
    //                    then cmdJmpNext or cmdJmpPrv
    //      help       -> printHelp
    //      quit, exit -> set quitRequested = true
    if(words[0]=="start"&&words.size()==2)return cmdStart(s, words[1]);
    if(words[0]=="next"&&words.size()==1)return cmdNext(s);
    if(words[0]=="prv"&&words.size()==1)return cmdPrv(s);
    if(words[0]=="jmp"&&words.size()==3){
        if(words[1]!= "next"&&words[1]!= "prv"){
            cout <<"unknown command" <<endl;
            printHelp();
            return 0;
        }
        int n=atoi(words[2].c_str());
        if(n<=0 || n >MAX_JUMP||to_string(n)!=words[2]){
            cout <<"Invalid jump number."<<endl;
            return 0;
        }
        if (words[1]=="next")return cmdJmpNext(s,n);
        return cmdJmpPrv(s,n);
    }
    if(words[0]=="help" && words.size()==1){
    printHelp();
    return  1;
    }
    // 4. For anything else, or a wrong number of words, print "unknown command" and the help text, and return false.
    cout <<"unknown command"<<endl;
    printHelp();
    return 0;
}
int main(int argc, char *argv[])
{
    // you get string LB_HOST and int LB_PORT = 0 in arguments of this main function. 
    if(argc!= 1&& argc!=3){
        cout<<"Usage: ./client [LB_HOST LB_PORT]"<<endl;
        return 1;
    }
    if(argc==3){
        LB_HOST=argv[1];
        char *end;
        long port=strtol(argv[2],&end,10);
        if (*end!=0||port<=0||port>65535){
            cout<<"Invalid load balancer port."<< endl;
            return 1;
        }
        LB_PORT=port;
    }
    // DebugSession session; initSession(session);
    DebugSession session;
    initSession(session);
    string line;
    // Print a short welcome and the help text.
    cout << "Welcome to Time-Travel Debugger!" << endl;
    printHelp();
    //
    // Infinite loop:
    //   if quitRequested, break
    while(1){
        cout << PROMPT;
        if(!getline(cin, line))break;
        bool quit = 0;
        runCommand(session, line, quit);
        if(quit)break;
    }
    // After the loop: closeSession(session) and return 0.
    closeSession(session);
    cout<<"Debugger closed."<<endl;
    return 0;
    // Never exit the loop because of an error. Only quit or the end of the input leaves it.

}