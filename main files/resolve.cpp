#include "validate.h"
#include"resolve.h"

int64_t writeResolveRecord(FILE *f, int64_t offsetField, const string &text)
{
    int64_t _offset=ftell(f);
    int32_t sizeString=text.size();
    fwrite(&offsetField,sizeof(int64_t),1,f);
    fwrite(&sizeString,sizeof(int32_t),1,f);
    for(int i=0;i<text.size();i++){
        fwrite(&text[i],sizeof(char),1,f);
    }
    return _offset;
    // writes one [offset(8B)][size(4B)][string] record at the current file position
    // returns this record's own starting byte position
}
int64_t readResolveRecord(FILE *f, string &outText)
{
    int64_t offset;
    int32_t size;
    fread(&offset,sizeof(int64_t),1,f);
    fread(&size,sizeof(int32_t),1,f);
    char t;
    outText="";
    for(int32_t i =0;i<size;i++){
        fread(&t,sizeof(char),1,f);
        outText+=t;
    }
    return offset;
    // reads one record at the current position and advances past it, returns the offset field - the raw line text comes back untouched in outText.
}
int64_t resolveProgram(const char *sourcePath, const char *resolveBinPath)
{
    FuncEntry funcArray[MAX_FUNCS];
    int32_t funcCount = 0;
    PendingPatch patches[MAX_PATCHES];
    int32_t patchCount = 0;

    int64_t offsetField=0;
    int64_t mainOffset=0;

    ifstream in(sourcePath);
    if(!in){
        throw "source file not found";
    }

    FILE* out=fopen(resolveBinPath,"wb");
    if(!out){
        throw "resolve file not found";
    }
    
    string line,w1,w2;
    bool mainFlag=0;
    while(!in.eof()){
        if(!readSourceLine(in,line))break;
        
        w1=firstWord(line);
        w2=secondWord(line);
        
        if(w1=="func"){
            if(w2=="main"&&mainFlag==0){
                mainFlag=1;
                mainOffset=offsetField;
            }else if(w2=="main"&&mainFlag)throw "Repeating main";
            funcArray[funcCount].funcName=secondWord(line);
            funcArray[funcCount++].byteOffsetInResolveBin=offsetField;
            if(funcCount>MAX_FUNCS)throw "Exceeded maxFuncton limit";
            int64_t _offset=writeResolveRecord(out,offsetField,line);    
            offsetField+=8+4+line.size();            
        }
        else if(w1=="call"){
            bool f=0;
            for(int i=0;i<funcCount;i++){
                if(w2==funcArray[i].funcName){
                    int64_t _offset=writeResolveRecord(out,funcArray[i].byteOffsetInResolveBin,line);    
                    offsetField+=8+4+line.size();
                    f=1;
                    break;
                }
            }
            if(f)continue;
            patches[patchCount].targetFuncName=w2;
            patches[patchCount++].byteOffsetOfOffsetField=offsetField;
            if(patchCount>MAX_PATCHES)throw "Exceeded maxPatch limit";
            int64_t _offset=writeResolveRecord(out,offsetField,line);    
            offsetField+=8+4+line.size();
            
        }
        else{
            int64_t _offset=writeResolveRecord(out,offsetField,line);    
            offsetField+=8+4+line.size();
        }
    }
    //main check
    if(!mainFlag)throw "main not found";
    //patching
    bool flag=0;
    for(int i=0;i<patchCount;i++){
        for(int j=0;j<funcCount;j++){
            if(patches[i].targetFuncName==funcArray[j].funcName){
                fseek(out,patches[i].byteOffsetOfOffsetField,SEEK_SET);
                fwrite(&funcArray[j].byteOffsetInResolveBin,sizeof(int64_t),1,out);
                flag=1;
                break;
            }
        }
        if(flag)flag=0;
        else throw "undeclared function called";


    }

    

    fclose(out);
    

    return mainOffset;

    // Every source line becomes one record holding the raw line, as-is.
    // resolve() only PEEKS at the leading word(s) -- enough to spot FUNC
    // (remember its position) and CALL (remember which function it needs
    // and where its offset field sits).
    // Once the whole file is written, every CALL's offset field is patched
    // with its target's position. Patching happens after the full write
    // Returns the byte offset of main's FUNC header record.
    // if there is no main return the error 
}
