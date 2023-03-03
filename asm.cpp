/*
Title: Assembler
Name: N.V. Vineeth(2001CS49)
Declaration of Authorship
This asm.cpp file is part of the assignment of CS322 at the 
department of Computer Science and Engg, IIT Patna.
*/
#include<bits/stdc++.h>

using namespace std;

struct str{
    string mnem;//The mnemonic name
    string op;//The operand value or name
    int optype;//Stores whether operand is unnecessary or if 'SET'(0), operand is text(1) or operand is number(stores base of operand)
    int pc;//Stores program counter
    string l;//Stores the line so we can understand the listing file/debug easier
};

vector<str> table;
map<string,int> symbols;
map<string,int> undeclaredsym;

void trim(string& l){
    int start=l.find_first_not_of(" \n\r\t\f\v");
    if(start !=string::npos)
        l=l.substr(start);
    int end=l.find_last_not_of(" \n\r\t\f\v");
    if(end!=string::npos)
        l=l.substr(0,end+1);
}

int LabelValidity(string s){
    if(!((s[0]>='a'&&s[0]<='z')||(s[0]>='A'&&s[0]<='Z'))){
        return 0;
    }
    for(int i=0;i<s.length();i++){
        if(!((s[i]>='a'&&s[i]<='z')||(s[i]>='A'&&s[i]<='Z')||(s[i]>='0'&&s[i]<='9')))
            return 0;
    }
    return 1;
}

int LabelDupe(string s){
    if(symbols.count(s)>0)
        return 0;
    return 1;
}

/*converts given number to hex*/
string formatfn(int x,int y){
    stringstream s;
    string l;
    if(y==0){
        if(x<0)
            x+=(1<<24);
        s<< setfill('0')<<setw(6)<<hex<<x;
    }
    else{
        unsigned int z=x;
        s<< setfill('0')<<setw(8)<<hex<<z;
    }
    return s.str();

}
/*  A function which reads a string and returns
    -1 = String is invalid
    1 = Variable name(Only alphabets and numbers allowed and doesn't start with 0)
    8 = Octal number
    10 = Decimal number
    16 = Hexadecimal number
*/    
int detectchar(string s){
    int base;
    if(s[0]!='0'){
       base=10;
       for(int i=0;i<s.length();i++)
        if((s[i]>='a'&&s[i]<='z')||(s[i]>='A'&&s[i]<='Z'))
            base=1;//it contains alphabets
        if(base==10){
            for(int i=0;i<s.length();i++)
                if(!((s[i]>='0'&&s[i]<='9')||(s[i]=='-')||(s[i]=='+')))
                    return -1;
            return 10;        
        }
        else {
            for(int i=0;i<s.length();i++)
                if(!((s[i]>='a'&&s[i]<='z')||(s[i]>='A'&&s[i]<='Z')||(s[i]>='0'&&s[i]<='9')))
                    return -1;
            return 1;
        }
    }
    else{
        if(s.length()==1)
            return 10;
        else{
            base=8;
            if(s[1]=='x')
                base=16;
            if(base==8){
                for(int i=0;i<s.length();i++)
                    if(!(s[i]>='0'&&s[i]<='7'))
                        return -1;
                return 8;
            }
            else {
                for(int i=2;i<s.length();i++)
                    if(!((s[i]>='a'&&s[i]<='e')||(s[i]>='A'&&s[i]<='E')||(s[i]>='0'&&s[i]<='9')))
                        return -1;
                return 16;
            }
        }
    }
}


int main() {
    int errors=0;
/*    cout<<"Enter the name of the file =>";
    string input;
    cin>>input;
*/
    string input="largest.asm";

    if(input.length()<=4){
        cout<<"Invalid filename";
        return 0;
    }
    string subs=input.substr(input.length()-4,4);
    if(subs!=".asm"){
    cout<<"Invalid filename";
        return 0;
    }
    ifstream asmfile;
    
    string filename=input.substr(0,input.find('.',0));
    
    //Now we declare all the opcodes. We map the mnemonics to a pair of opcode and operand. the operands being represented by integers according to the following format.
    //0--No operand     1--target_label operand       2--value operand
    map<string,pair<string,int>> ops;
    ops["data"]={"",2};
    ops["ldc"]={"00",2};
    ops["adc"]={"01",2};
    ops["ldl"]={"02",1};
    ops["stl"]={"03",1};
    ops["ldnl"]={"04",1};
    ops["stnl"]={"05",1};
    ops["add"]={"06",0};
    ops["sub"]={"07",0};
    ops["shl"]={"08",0};
    ops["shr"]={"09",0};
    ops["adj"]={"0a",2};
    ops["a2sp"]={"0b",0};
    ops["sp2a"]={"0c",0};
    ops["call"]={"0d",1};
    ops["return"]={"0e",0};
    ops["brz"]={"0f",1};
    ops["brlz"]={"10",1};
    ops["br"]={"11",1};
    ops["HALT"]={"12",0};
    ops["SET"]={"",2};


    ofstream logfile(filename + ".L");
    asmfile.open(input);
    if(asmfile.fail()){
        logfile<<input.substr(0,input.find('.',0))<<".asm file doesn't exist."<<endl;
        logfile.close();
        return 0;
    }

    /*----------------------------------------------------First Pass----------------------------------------------------*/
    //First pass to define symbols and literals, to store them in a table and to keep track of location counter

    int pc=-1,lno=0;

    while(!asmfile.eof()){
        lno++;
        string l,label,mnemonic,operand;
        getline(asmfile,l);
        int semicolon=l.find(";");
        if(semicolon!=string::npos)
            l=l.substr(0,semicolon);
        trim(l);
        //Now string 'l' does not contain comments.
        /*
        Step1:  if a colon(':') is present, everything before that will be trimmed and stored
                in string 'label' and everything after will be trimmed and stored in string 'mnemonic'.
        */
        if(l.length()==0)
            continue;
        int colon=l.find(":");
        if(colon!=string::npos){
            label=l.substr(0,colon);
            trim(label);
            mnemonic=l.substr(colon+1,l.length()-colon);
            trim(mnemonic);
        }
        //If no colon is present, everything will be stored in 'mnemonic' by default.
        else{
            mnemonic=l;
        }
        if(mnemonic.length()>0)
            pc++;
        else {            
            if(LabelValidity(label)==0){
                logfile<<"Label name has to be alphanumeric with the first letter as alphabet. Violation in line "<<lno<<endl;
                errors++;
            }
            if(LabelDupe(label)==0){
                logfile<<"Label name has to be unique. Violation in line "<<lno<<endl;
                errors++;
            }
            table.push_back({label,"",0,pc+1,l});
            symbols.insert({label,pc+1});
            undeclaredsym.erase(label);
            continue;
        }

        /*Step 2:   In the string 'mnemonic' will search for spaces or '\t'. If we detect it, we will store
                    the trimmed version of everything after in string 'operand'. If not, string operand will
                    remain empty.*/
        int space=mnemonic.find(" ");
        if(space==string::npos)
            space=mnemonic.find("\t");
        if(space!=string::npos){
            operand=mnemonic.substr(space+1,mnemonic.length()-space);
            trim(operand);
            mnemonic=mnemonic.substr(0,space);
            trim(mnemonic);
        }
        if(label.length()>0){

            if(mnemonic=="SET"){
                if(operand.length()==0) {
                    logfile<<"SET instruction has no operand in line "<<lno<<endl;
                    errors++;
                }
                else {
                    if(detectchar(operand) <2) { //Operand is not a number
                        logfile<<"SET instruction has invalid operand at line "<<lno<<endl;
                        errors++;
                    }
                    else{
                        table.push_back({"SET",operand,0,pc,l});
                        symbols.insert({label,stoi(operand,0,detectchar(operand))});
                        undeclaredsym.erase(label);
                    }
                }
            }
            else{
                symbols.insert({label,pc});
                undeclaredsym.erase(label);
            }
        
        }

        auto it=ops.find(mnemonic);
        if(it==ops.end()){
            logfile<<"Invalid mnemonic at line "<<lno<<endl;
            errors++;
            continue;
        }
        if(!((operand.length()==0 && ops[mnemonic].second==0) ||(operand.length()>0 && ops[mnemonic].second>0))){
            if(operand.length()==0 && ops[mnemonic].second>0){
                logfile<<"Missing operand at "<<lno<<endl;
                errors++;
            }
            else if(operand.length()>0 && ops[mnemonic].second==0){
                logfile<<"Unexpected operand at "<<lno<<endl;
                errors++;
            }
        }
        if(mnemonic!="SET"){
            if(operand.length()==0)
                table.push_back({mnemonic,operand,0,pc,l});
            else
                table.push_back({mnemonic,operand,detectchar(operand),pc,l});
        }

        if(detectchar(operand)==-1){
            int comma=operand.find(",",0);
            int space=operand.find(" ",0);
            if(comma!=string::npos ||space!=string::npos){
                logfile<<"Extra operand found at line "<<lno<<endl;
                errors++;
            }
            else{
                logfile<<"Invalid operand at line "<<lno<<endl;
                errors++;
            }
        }
        else if (detectchar(operand)==1 ){//Operand is a symbol
            auto it=symbols.find(operand);
            if(it==symbols.end())
                undeclaredsym.insert({operand,lno});
        }
    }
    map<string,int>::iterator itr;
    for(itr=undeclaredsym.begin();itr!=undeclaredsym.end();itr++){
        logfile<<"WARNING: Unused label "<<itr->first<<" found at line "<<itr->second<<endl;
    }
    if(errors>0){
        logfile<<errors<<" errors detected. Program terminated."<<endl;
    }
    else{
        /*----------------------------------------------------Second Pass----------------------------------------------------*/
        //Second pass to generate object code
        logfile<<"Your program is free of errors.";
        ofstream listingfile(filename+".lst");
        ofstream objectfile(filename+".O",ios::binary | ios::out);
        for(int i=0;i<table.size();i++){
            string towrite;
            if(table[i].optype == 0 ){
                towrite="        ";//8 spaces
                if(table[i].mnem != "SET" && symbols.count(table[i].mnem)==0){    
                    towrite="000000";
                    towrite.append(ops[table[i].mnem].first);
                }

                listingfile<<formatfn(table[i].pc,1)<<" "<<towrite <<" "<<table[i].l<<endl;
                                
                if (towrite.empty() or towrite == "        ")
                    continue;
                stringstream s;
                s << hex << towrite;
                unsigned int x;
                s >> x; // output it as a signed type
                static_cast<int>(x);
                objectfile.write((const char *)&x,sizeof(unsigned int));
            }
            else if(table[i].optype == 1){//If operand is text
                if(ops[table[i].mnem].second==1){//operand is offset
                    int target_label=symbols[table[i].op]-table[i].pc-1;
                    towrite=formatfn(target_label,0)+ops[table[i].mnem].first;
                    listingfile<<formatfn(table[i].pc,1)<<" "<<towrite<<" "<<table[i].l<<endl;

                    if(towrite.empty() ||towrite=="        ")
                        continue;
                    stringstream s;
                    s << hex << towrite;
                    unsigned int x;
                    s >> x; // output it as a signed type
                    static_cast<int>(x);
                    objectfile.write((const char *)&x,sizeof(unsigned int));
                }
                else{
                    towrite=formatfn(symbols[table[i].op],0) + ops[table[i].mnem].first;
                    listingfile<<formatfn(table[i].pc,1)<<" "<<towrite<<" "<<table[i].l<<endl;

                    if(towrite.empty() || towrite=="        ")
                        continue;
                    stringstream s;
                    s << hex << towrite;;
                    unsigned int x;
                    s >> x; // output it as a signed type
                    static_cast<int>(x);
                    objectfile.write((const char *)&x,sizeof(unsigned int)); 

                }
            }
            else{
                int t;
                int temp=stoi(table[i].op,0,table[i].optype);
                if(table[i].mnem == "data")
                    t=1;
                else
                    t=0;
                towrite=formatfn(temp, t) + ops[table[i].mnem].first;
                listingfile<<formatfn(table[i].pc,1)<<" "<<towrite<<" "<<table[i].l<<endl;

                if(towrite.empty() || towrite=="        ")
                        continue;
                    stringstream s;
                    s << hex << towrite;
                    unsigned int x;
                    s >> x; // output it as a signed type
                    static_cast<int>(x);
                    objectfile.write((const char *)&x,sizeof(unsigned int));
            }

        }
        listingfile.close();
        objectfile.close();
    }
    logfile.close();
/*    map<string,int>::iterator it;
    for(it=symbols.begin();it!=symbols.end();it++)
        cout<<it->first<<" "<<it->second<<endl;
    for(it=undeclaredsym.begin();it!=undeclaredsym.end();it++)
        cout<<endl<<it->first<<" "<<it->second;
*/
    return 0;
}