#include"serialize.h"
void writeHeader(FILE *f, const TTDBHeader &h)
{
    fwrite(h.magic, 1, 4, f);
    fwrite(&h.version, sizeof(int32_t), 1, f);
    fwrite(&h.stepCount, sizeof(int32_t), 1, f);
    fwrite(&h.indexOffset, sizeof(int64_t), 1, f);

}
void writeTdbg(Timeline &timeline, const char *tdbgPath)
{
    TTDBHeader header;
    
    int32_t stepcount =timeline.getStepCount();
    header.stepCount=stepcount;
    
    FILE* ttdb = fopen(tdbgPath,"wb");
    if(!ttdb) throw "sessoin file not found";

    writeHeader(ttdb,header);

    // placeholder for header
    int64_t *idx = new int64_t [stepcount]; 
    
    // index array of the size of stepcount from the timeline
    TimelineNode *traversePtr = timeline.begin();
    // placing each snapshot in the file while maintaining the index(starting point of each nth snapshot)
    for(int i=0;i<stepcount;i++){
        if(traversePtr==nullptr) throw"timeline traverse failed";
        idx[i]=ftell(ttdb);

        fwrite(&traversePtr->data->stackDepth,sizeof(int32_t),1,ttdb);

        for(int j=0; j<traversePtr->data->stackDepth;j++){
            
            Frame & tempFrame=traversePtr->data->callStack[j];
            int32_t tempSize;
            
            tempSize=static_cast<int32_t>(tempFrame.func_name.size());
            fwrite(&tempSize,sizeof(int32_t),1,ttdb);
            fwrite(tempFrame.func_name.data(),sizeof(char),tempSize,ttdb);
            fwrite(&tempFrame.argc,sizeof(int32_t),1,ttdb);

            for(int k=0;k<tempFrame.argc;k++){
                tempSize=static_cast<int32_t>(tempFrame.argv[k].name.size());
                fwrite(&tempSize,sizeof(int32_t),1,ttdb);
                fwrite(tempFrame.argv[k].name.data(),sizeof(char),tempSize,ttdb);
                fwrite(&tempFrame.argv[k].value,sizeof(int32_t),1,ttdb);
            }
            fwrite(&tempFrame.returnLine,sizeof(int64_t),1,ttdb);
            fwrite(&tempFrame.localCount,sizeof(int32_t),1,ttdb);

            for(int k=0;k<tempFrame.localCount;k++){
                tempSize=static_cast<int32_t>(tempFrame.locals[k].name.size());
                fwrite(&tempSize,sizeof(int32_t),1,ttdb);
                fwrite(tempFrame.locals[k].name.data(),sizeof(char),tempSize,ttdb);
                fwrite(&tempFrame.locals[k].value,sizeof(int32_t),1,ttdb);
            }
        }    
        traversePtr=traversePtr->next;
        
    }
    header.indexOffset= ftell(ttdb);
    for(int i=0;i<stepcount;i++){
        fwrite(&idx[i],sizeof(int64_t),1,ttdb);
    }
    // after timeline add the index array i the file
    fseek(ttdb,0,SEEK_SET);
    writeHeader(ttdb,header);
    delete [] idx;
    fclose(ttdb);
    // update the header
}