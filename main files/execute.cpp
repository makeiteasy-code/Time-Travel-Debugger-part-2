#include"execute.h"
int32_t tokenizeLine(const string &line, Token tokens[], int32_t maxTokens)
{
    int i=0;
    int count=0;
    string word="";
    if(count>=maxTokens)return count;
    while(line[i]==' ')i++;
    while(line[i]!=' '&&line[i]!=0)word+=line[i++];
    tokens[count].type=KEYWORD;
    tokens[count].text=word;
    count++;

    if(word=="func_end")return count;
    
    word="";
    if(count>=maxTokens)return count;
    while(line[i]==' ')i++;
    while(line[i]!=' '&&line[i]!=0)word+=line[i++];
    tokens[count].type=IDENTIFIER;
    tokens[count].text=word;
    count++;
    while(line[i]!=0){
        word="";
        while(line[i]==' ')i++;
        if(line[i]==0)break;
        if(count>=maxTokens)throw "max Token already reached";
        while(line[i]!=' '&&line[i]!=0)word+=line[i++];
        tokens[count].type=PARAM;
        tokens[count].text=word;
        count++;
    }
    return count;
    // first word is always a instruction keyword
    // instruction set = [func, func_end, call, set, add, sub, mul and div]
    // next word is identifier like name of a function, variable name
    // after identifier all are the params/arg, space separated
}
Snapshot *buildSnapshot(Stack<Frame> &callStack)
{
    Snapshot* snapsnap=new Snapshot;
    snapsnap->stackDepth=callStack.snapshot_into(snapsnap->callStack,MAX_STACK_DEPTH);
    return snapsnap;
    // build the snapshot based on the callStack given
}
void executeProgram(const char *resolveBinPath, int64_t mainOffset, Timeline &timeline)
{
    FILE* in = fopen(resolveBinPath,"rb");
    if(in==nullptr)throw "resolve file not found";
    if(fseek(in,mainOffset,SEEK_SET))throw "main not found in resolve file";

    Stack<Frame> callStack;

    Frame mainFrame("main",0,-1,0);
    
    callStack.push(mainFrame);
    
    string line;
    readResolveRecord(in,line);
    cout<<line<<endl;
    
    
    Token tokens[MAX_TOKENS];
    int32_t tokenCount=0;
    while(true){
        int64_t lineOffsetByte = ftell(in);
        int64_t offsetbyte = readResolveRecord(in,line);
        cout<<line<<endl;
        tokenCount = tokenizeLine(line,tokens,MAX_TOKENS);
        string key =tokens[0].text;
        timeline.record(buildSnapshot(callStack));
        if(key=="set"){
            if(tokenCount!=3)throw "wrong set instruction";
            Frame &current=callStack.peek();
            string var=tokens[1].text;
            int32_t value=stoi(tokens[2].text);
            bool flag=0;
            for(int i=0;i<current.argc;i++){
                if(current.argv[i].name==var){
                    current.argv[i].value=value;
                    flag=1;
                    break;
                }
            }
            if(flag)continue;
            for(int i=0;i<current.localCount;i++){
                if(current.locals[i].name==var){
                    current.locals[i].value=value;
                    flag=1;
                    break;
                }
            }
            if(flag)continue;
            
            if(current.localCount>=MAX_VARS_PER_FRAME)throw "cant add more variable";
            current.locals[current.localCount].name=var;
            current.locals[current.localCount++].value=value;
            
        }
        else if(key=="add"){
            if(tokenCount!=3)throw "wrong add instruction";
            Frame &current=callStack.peek();
            string var1=tokens[1].text;
            string var2=tokens[2].text;
            bool flag1=0;
            bool flag2=0;
            Variable* Var1;
            Variable* Var2;
            for(int i=0;i<current.argc;i++){
                if(current.argv[i].name==var1){
                    flag1=1;
                    Var1= &current.argv[i];
                }
                if(current.argv[i].name==var2){
                    Var2= &current.argv[i];
                    flag2=1;
                }
                if(flag1&&flag2)break;
            }
            if(flag1&&flag2){
                Var1->value+=Var2->value;
                continue;
            }
            for(int i=0;i<current.localCount;i++){
                if(current.locals[i].name==var1){
                    Var1= &current.locals[i];
                    flag1=1;
                }
                if(current.locals[i].name==var2){
                    Var2= &current.locals[i];
                    flag2=1;
                }
                if(flag1&&flag2)break;
            }
             if(flag1&&flag2){
                Var1->value+=Var2->value;
                continue;
            }
             if(!(flag1&&flag2)){
                throw "Variabes not found";
                
            }

        }
        else if(key=="sub"){
            if(tokenCount!=3)throw "wrong sub instruction";
            Frame &current=callStack.peek();
            string var1=tokens[1].text;
            string var2=tokens[2].text;
            bool flag1=0;
            bool flag2=0;
            Variable* Var1;
            Variable* Var2;
            for(int i=0;i<current.argc;i++){
                if(current.argv[i].name==var1){
                    flag1=1;
                    Var1= &current.argv[i];
                }
                if(current.argv[i].name==var2){
                    Var2= &current.argv[i];
                    flag2=1;
                }
                if(flag1&&flag2)break;
            }
            if(flag1&&flag2){
                Var1->value-=Var2->value;
                continue;
            }
            for(int i=0;i<current.localCount;i++){
                if(current.locals[i].name==var1){
                    Var1= &current.locals[i];
                    flag1=1;
                }
                if(current.locals[i].name==var2){
                    Var2= &current.locals[i];
                    flag2=1;
                }
                if(flag1&&flag2)break;
            }
             if(flag1&&flag2){
                Var1->value-=Var2->value;
                continue;
            }
             if(!(flag1&&flag2)){
                throw "Variabes not found";
                
            }

        }
        else if(key=="mul"){
            if(tokenCount!=3)throw "wrong add instruction";
            Frame &current=callStack.peek();
            string var1=tokens[1].text;
            string var2=tokens[2].text;
            bool flag1=0;
            bool flag2=0;
            Variable* Var1;
            Variable* Var2;
            for(int i=0;i<current.argc;i++){
                if(current.argv[i].name==var1){
                    flag1=1;
                    Var1= &current.argv[i];
                }
                if(current.argv[i].name==var2){
                    Var2= &current.argv[i];
                    flag2=1;
                }
                if(flag1&&flag2)break;
            }
            if(flag1&&flag2){
                Var1->value*=Var2->value;
                continue;
            }
            for(int i=0;i<current.localCount;i++){
                if(current.locals[i].name==var1){
                    Var1= &current.locals[i];
                    flag1=1;
                }
                if(current.locals[i].name==var2){
                    Var2= &current.locals[i];
                    flag2=1;
                }
                if(flag1&&flag2)break;
            }
            if(flag1&&flag2){
                Var1->value*=Var2->value;
                continue;
            }
             if(!(flag1&&flag2)){
                throw "Variabes not found";
                
            }

        }
        else if(key=="div"){
            if(tokenCount!=3)throw "wrong add instruction";
            Frame &current=callStack.peek();
            string var1=tokens[1].text;
            string var2=tokens[2].text;
            bool flag1=0;
            bool flag2=0;
            Variable* Var1;
            Variable* Var2;
            for(int i=0;i<current.argc;i++){
                if(current.argv[i].name==var1){
                    flag1=1;
                    Var1= &current.argv[i];
                }
                if(current.argv[i].name==var2){
                    Var2= &current.argv[i];
                    if(current.argv[i].value==0)throw"IllEGAL OPERATION DIVIDING BY ZERO";
                    flag2=1;
                }
                if(flag1&&flag2)break;
            }
            if(flag1&&flag2){
                Var1->value/=Var2->value;
                continue;
            }
            for(int i=0;i<current.localCount;i++){
                if(current.locals[i].name==var1){
                    Var1= &current.locals[i];
                    flag1=1;
                }
                if(current.locals[i].name==var2){
                    Var2= &current.locals[i];
                    if(current.locals[i].value==0)throw"IllEGAL OPERATION DIVIDING BY ZERO";
                    flag2=1;
                }
                if(flag1&&flag2)break;
            }
             if(flag1&&flag2){
                Var1->value/=Var2->value;
                continue;
            }
             if(!(flag1&&flag2)){
                throw "Variabes not found";
                
            }

        }
        else if(key=="func"){
            continue;
        }
        else if(key=="func_end"){
            if(callStack.depth()==1){
                break;
            }
            
            int64_t callOffset=callStack.peek().returnLine;
            Frame poped=callStack.pop();
            Frame &current=callStack.peek();

            if(fseek(in,callOffset,SEEK_SET)) throw"cant go back to caller";

            offsetbyte = readResolveRecord(in,line);
            tokenCount = tokenizeLine(line,tokens,MAX_TOKENS);
            
            for(int i=0;i<poped.argc;i++){
                bool flag=0;
                for(int j=0;j<current.argc;j++){
                    if(current.argv[j].name==tokens[i+2].text){
                        current.argv[j].value=poped.argv[i].value;
                        flag=1;
                        break;
                        
                    }
                }
                if(flag)continue;
                for(int j=0;j<current.localCount;j++){
                    if(current.locals[j].name==tokens[i+2].text){
                        current.locals[j].value=poped.argv[i].value;
                        flag=1;
                        break;
                        
                    }
                }
                if(!flag)throw "Call argument not found";
            
            }
            
            
        }
        else if(key=="call"){
            if(tokenCount<2) throw "invalid instruction";
            if(tokenCount-2>MAX_VARS_PER_FRAME)throw "can add more variable than the max cap";
            int32_t newArgV[MAX_VARS_PER_FRAME];
            int32_t newArgCount=0;
            bool flag=0;
            
            for(int i=0;i<tokenCount-2;i++){
                flag=0;
                string tempVar=tokens[i+2].text;
                Frame &current=callStack.peek();
                for(int j=0;j<current.argc;j++){
                    if(current.argv[j].name==tempVar){
                        newArgV[newArgCount++]=current.argv[j].value;
                        flag=1;
                        break;
                    }
                }
                if(flag)continue;
                for(int j=0;j<current.localCount;j++){
                    if(current.locals[j].name==tempVar){
                        newArgV[newArgCount++]=current.locals[j].value;
                        flag=1;
                        break;
                    }
                }
                if(!flag)throw "Call argument not found";
            }
            
            int32_t previousCallTokenCount=tokenCount;
            
            
            if(fseek(in,offsetbyte,SEEK_SET))throw "function not found";
            offsetbyte = readResolveRecord(in,line);
            tokenCount=tokenizeLine(line,tokens,MAX_TOKENS);
            if(previousCallTokenCount!=tokenCount)throw"call paramerters dont match function parameter";
            
            Frame temp(tokens[1].text,tokenCount-2,lineOffsetByte,0);
            
            for(int i = 0; i<tokenCount-2;i++){
                temp.argv[i].name=tokens[i+2].text;
                temp.argv[i].value=newArgV[i];
            }
            callStack.push(temp);
        }
        else{
            throw "Wrong keyword used";
        }




    }




}