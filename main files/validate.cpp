#include"validate.h"
bool readSourceLine(ifstream &in, string &out)
{
   do{
        if(getline(in,out)&&!(out==""))
            return 1;
    } while(out==""&&!in.eof());
    return 0;
}
string firstWord(const string &line)
{
    string word;
    for(int i =0;line[i]!=' '&&line[i]!=0&&line[i]!='\t';i++)word+=line[i];
    return word;
    
}
string secondWord(const string &line)
{
    string word2;
    int i=0;
    while(line[i]!=' '&&line[i]!=0&&line[i]!='\t'){
        i++;
    }
    if(line[i]==0)return "";
    i++;
    while(line[i]!=' '&&line[i]!=0&&line[i]!='\t'){
        word2+=line[i];
        i++;
    }
    return word2;
}
bool validateProgram(const char *sourcePath)
{
    ifstream in(sourcePath);
    bool flag=0;
    string w1;
    string w2;
    while(!in.eof()){
        string line;
        if(!readSourceLine(in,line))break;
        
        w1=firstWord(line);
        
        if(w1=="func"&&(!flag)) {
            flag=1;
            w2=secondWord(line);
            if(w2=="")return 0;
        }
        else if(w1=="func"&& flag) return 0;
        else if(w1=="func_end"&&(!flag))return 0;
        if(w1=="func_end"&&flag){
            flag=0;
        }

    }
    if(flag)return 0;
    return 1;
    // for each func defined there should be exactly one func_end and no nested funcs allowed - 
}
