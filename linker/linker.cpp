#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<iomanip>

using namespace std;

struct symbols {
    string symbolName;
    int symbolAddress;
    bool used;
    int module;
    bool error;
    string errorMessage;
};

struct memMap {
    int adr1;
    int adr2;
    int module;
    bool error;
    string errorMessage;
};

struct moduleErr {
    int pass;
    int module;
    string errorMessage;
};

vector<int> moduleAddress;
vector<symbols> symbolTable;
vector<memMap> memoryMap;
vector<moduleErr> moduleError;

int currIndex = 0;
int currModule = 0;
int currAddress = 0;
int maxModuleAddress = 0;
int fileLine = 1;
int fileIndex = 0;
int symFlag = 1;

string str = "";
string errorMsgs = "";
int stringSize = 0;

void handleErrors(string syntaxError){
    cout<<syntaxError<<endl;
    exit(0);
}


int numOfDigits(int n){
    int count=0;
    while(n!=0){
        n=n/10;
        count++;
    }
    return count;
}

int getNextNumber(){
    if(currIndex<stringSize && !(str[currIndex]==' '||str[currIndex]=='\t'||str[currIndex]=='\n'||isdigit(str[currIndex])||isalpha(str[currIndex]))){
        string offset;
        if(fileLine==1){
            offset = to_string(fileIndex+1);
        }
        else{
            offset = to_string(fileIndex);
        }
        string parse1 = "Parse Error line "+to_string(fileLine)+" offset "+offset+": NUM_EXPECTED";
        handleErrors(parse1);
    }
    if(currIndex >= stringSize){
        return -1;
    }
    int prevLine = fileLine;
    int prevIndex = fileIndex;
    while(currIndex<stringSize && !isdigit(str[currIndex]) && !isalpha(str[currIndex])){
        if(fileIndex!=0){
            prevIndex = fileIndex+1;
        }
        if(str[currIndex]=='\n'){
            prevLine = fileLine;
            prevIndex = fileIndex;
            fileLine++;
            fileIndex = 0;
        }
        if(currIndex<stringSize && !(str[currIndex]==' '||str[currIndex]=='\t'||str[currIndex]=='\n'||isdigit(str[currIndex])||isalpha(str[currIndex]))){
            string offset;
            if(fileLine==1){
                offset = to_string(fileIndex+1);
            }
            else{
                offset = to_string(fileIndex);
            }
            string parse1 = "Parse Error line "+to_string(fileLine)+" offset "+offset+": NUM_EXPECTED";
            handleErrors(parse1);
        }
        currIndex++;
        fileIndex++;
    }
    if(isalpha(str[currIndex])){
        string offset;
        if(fileLine==1){
            offset = to_string(fileIndex+1);
        }
        else{
            offset = to_string(fileIndex);
        }
        string parse1 = "Parse Error line "+to_string(fileLine)+" offset "+offset+": NUM_EXPECTED";
        handleErrors(parse1);
        return getNextNumber();
    }
    else if(isdigit(str[currIndex])){
        int num = 0;
        while(currIndex<stringSize && isdigit(str[currIndex])){
            if(num >= 214748364 && (str[currIndex] - 48)>7){
                string offset;
                if(fileLine==1){
                    offset = to_string(fileIndex + 1 - numOfDigits(num));
                }
                else{
                    offset = to_string(fileIndex - numOfDigits(num));
                }
                string parse1 = "Parse Error line "+to_string(fileLine)+" offset "+offset+": NUM_EXPECTED";
                handleErrors(parse1);
            }
            num = 10*num + (str[currIndex] - 48);
            if(str[currIndex]=='\n'){
                fileLine++;
                fileIndex = 0;
            }
            currIndex++;
            fileIndex++;
        }
        if(currIndex<stringSize && !(str[currIndex]==' '||str[currIndex]=='\t'||str[currIndex]=='\n'||isdigit(str[currIndex])||isalpha(str[currIndex]))){
            string offset;
            if(fileLine==1){
                offset = to_string(fileIndex+1-numOfDigits(num));
            }
            else{
                offset = to_string(fileIndex-numOfDigits(num));
            }
            string parse1 = "Parse Error line "+to_string(fileLine)+" offset "+offset+": NUM_EXPECTED";
            handleErrors(parse1);
        }
        return num;
    }
    else{
        fileLine = prevLine;
        if(fileLine==1){
            fileIndex = prevIndex+1;
        }
        else{
            fileIndex = prevIndex;
        }
        return -1;
    }
}

string getNextSymbol(){
    if(currIndex<stringSize && !(str[currIndex]==' '||str[currIndex]=='\t'||str[currIndex]=='\n'||isdigit(str[currIndex])||isalpha(str[currIndex]))){
        string offset;
        if(fileLine==1){
            offset = to_string(fileIndex+1);
        }
        else{
            offset = to_string(fileIndex);
        }
        string expectation;
        if(symFlag==1){
            expectation = "SYM";
        }
        else{
            expectation = "ADDR";
        }
        string parse1 = "Parse Error line "+to_string(fileLine)+" offset "+offset+": "+expectation+"_EXPECTED";
        handleErrors(parse1);
    }
    if(currIndex >= stringSize){
        return "-1";
    }
    int prevLine = fileLine;
    int prevIndex = fileIndex;
    while(currIndex<stringSize && !isdigit(str[currIndex]) && !isalpha(str[currIndex])){
        if(fileIndex!=0){
            prevIndex = fileIndex+1;
        }
        if(str[currIndex]=='\n'){
            prevLine = fileLine;
            prevIndex = fileIndex;
            fileLine++;
            fileIndex = 0;
        }
        if(currIndex<stringSize && !(str[currIndex]==' '||str[currIndex]=='\t'||str[currIndex]=='\n'||isdigit(str[currIndex])||isalpha(str[currIndex]))){
            string offset;
            if(fileLine==1){
                offset = to_string(fileIndex+1);
            }
            else{
                offset = to_string(fileIndex);
            }
            string expectation;
            if(symFlag==1){
                expectation = "SYM";
            }
            else{
                expectation = "ADDR";
            }
            string parse1 = "Parse Error line "+to_string(fileLine)+" offset "+offset+": "+expectation+"_EXPECTED";
            handleErrors(parse1);
        }
        currIndex++;
        fileIndex++;
    }
    if (isdigit(str[currIndex])){
        string offset;
        if(fileLine==1){
            offset = to_string(fileIndex+1);
        }
        else{
            offset = to_string(fileIndex);
        }
        string expectation;
        if(symFlag==1){
            expectation = "SYM";
        }
        else{
            expectation = "ADDR";
        }
        string parse2 = "Parse Error line "+to_string(fileLine)+" offset "+offset+": "+expectation+"_EXPECTED";
        handleErrors(parse2);
        return getNextSymbol();
    }
    else if(isalpha(str[currIndex])){
        string sym = "";
        while(currIndex<stringSize && (isalpha(str[currIndex]) || isdigit(str[currIndex]))){
            sym = sym + str[currIndex];
            if(str[currIndex]=='\n'){
                fileLine++;
                fileIndex = 0;
            }
            currIndex++;
            fileIndex++;
        }
        if(currIndex<stringSize && !(str[currIndex]==' '||str[currIndex]=='\t'||str[currIndex]=='\n'||isdigit(str[currIndex])||isalpha(str[currIndex]))){
            string offset;
            if(fileLine==1){
                offset = to_string(fileIndex+1-sym.size());
            }
            else{
                offset = to_string(fileIndex-sym.size());
            }
            string expectation;
            if(symFlag==1){
                expectation = "SYM";
            }
            else{
                expectation = "ADDR";
            }
            string parse1 = "Parse Error line "+to_string(fileLine)+" offset "+offset+": "+expectation+"_EXPECTED";
            handleErrors(parse1);
        }
        return sym;
    }
    else{
        fileLine = prevLine;
        if(fileLine==1){
            fileIndex = prevIndex+1;
        }
        else{
            fileIndex = prevIndex;
        }
        return "-1";
    }

}

void addToSymbolTable(string sym, int val, bool errExists, string errMsg){
    int flag = 0;
    for(int i=0;i<symbolTable.size();i++){
        if(symbolTable.at(i).symbolName.compare(sym)==0){
            symbolTable.at(i).error = true;
            string err2 = "Error: This variable is multiple times defined; first value used";
            symbolTable.at(i).errorMessage = err2;
            flag=1;
        }
    }
    if(flag==0){
        symbols temp;
        temp.symbolName = sym;
        temp.symbolAddress = val;
        temp.used = false;
        temp.module = currModule;
        temp.error = errExists;
        temp.errorMessage = errMsg;
        symbolTable.push_back(temp);
    }

}

void addToMemoryTable(int memAdr, int actAdr, bool errExists, string errMsg){
    memMap temp;
    temp.adr1 = memAdr;
    temp.adr2 = actAdr;
    temp.module = currModule;
    temp.error = errExists;
    temp.errorMessage = errMsg;
    memoryMap.push_back(temp);
}

void addToModuleError(int currPass, int currMod, string errMsg){
    int flag = 0;
    for(int i=0;i<moduleError.size();i++){
        if(moduleError.at(i).pass == currPass && moduleError.at(i).module == currMod){
            moduleError.at(i).errorMessage = moduleError.at(i).errorMessage+errMsg;
            flag=1;
        }
    }
    if(flag==0){
        moduleErr temp;
        temp.pass = currPass;
        temp.module = currMod;
        temp.errorMessage = errMsg;
        moduleError.push_back(temp);
    }
}

void passOne(){
    int line = 1;
    moduleAddress.push_back(0);
    while(currIndex<stringSize){
        if(line==1){
            symFlag = 1;
            int defCount = getNextNumber();
            int prevIndex = fileIndex - numOfDigits(defCount);
            if(fileLine==1){
                prevIndex=prevIndex+1;
            }
            if(defCount>=16){
                string parse5 = "Parse Error line "+to_string(fileLine)+" offset "+to_string(prevIndex)+": TOO_MANY_DEF_IN_MODULE";
                handleErrors(parse5);
            }
            int i=0;
            while(i<defCount){
                string symbol = getNextSymbol();
                
                if(symbol.size()>=16){
                    string parse4 = "Parse Error line "+to_string(fileLine)+" offset "+to_string(fileIndex-symbol.size())+": SYM_TOO_LONG";
                    handleErrors(parse4);
                }
                if(symbol.compare("-1")==0){
                    string parse2 = "Parse Error line "+to_string(fileLine)+" offset "+to_string(fileIndex)+": SYM_EXPECTED";
                    handleErrors(parse2);
                }
                int val = getNextNumber();
                if(val==-1){
                    string parse1 = "Parse Error line "+to_string(fileLine)+" offset "+to_string(fileIndex-numOfDigits(val))+": NUM_EXPECTED";
                    handleErrors(parse1);
                }
                val = val+moduleAddress.at(currModule);
                addToSymbolTable(symbol,val,false,"No Error");
                i++;
            }
            line++;
        }
        else if(line==2){
            symFlag = 1;
            int symCount = getNextNumber();
            int prevIndex = fileIndex - numOfDigits(symCount);
            if(symCount>=16){
                string parse6 = "Parse Error line "+to_string(fileLine)+" offset "+to_string(prevIndex)+": TOO_MANY_USE_IN_MODULE";
                handleErrors(parse6);
            }
            int i=0;
            while(i<symCount){
                string sym = getNextSymbol();
                if(sym.size()>=16){
                    string parse4 = "Parse Error line "+to_string(fileLine)+" offset "+to_string(fileIndex-sym.size())+": SYM_TOO_LONG";
                    handleErrors(parse4);
                }
                if(sym.compare("-1")==0){
                    string parse2 = "Parse Error line "+to_string(fileLine)+" offset "+to_string(fileIndex)+": SYM_EXPECTED";
                    handleErrors(parse2);
                }
                i++;
            }
            line++;
        }
        else if(line==3){
            symFlag = 2;
            int adrCount = getNextNumber();
            int prevIndex = fileIndex - numOfDigits(adrCount);
            if(moduleAddress.at(currModule) + adrCount>=512){
                string parse7 = "Parse Error line "+to_string(fileLine)+" offset "+to_string(prevIndex)+": TOO_MANY_INSTR";
                handleErrors(parse7);
            }
            for(int i=0;i<symbolTable.size();i++){
                if(symbolTable.at(i).module==currModule
                            && symbolTable.at(i).symbolAddress-moduleAddress.at(currModule)>=adrCount){
                    string err5 = "Warning: Module "+to_string(currModule+1) + ": "
                                + symbolTable.at(i).symbolName + " too big "
                                + to_string(symbolTable.at(i).symbolAddress-moduleAddress.at(currModule))
                                + " (max=" + to_string(adrCount-1) + ") assume zero relative\n";
                    cout<<err5;
                    symbolTable.at(i).symbolAddress = moduleAddress.at(currModule);
                }
            }
            int i=0;
            while(i<adrCount){
                string symbol = getNextSymbol();
                if((symbol.compare("A")!=0)&&(symbol.compare("E")!=0)&&(symbol.compare("I")!=0)&&(symbol.compare("R")!=0)){
                    string parse3;
                    if(symbol=="-1"){
                        parse3 = "Parse Error line "+to_string(fileLine)+" offset "+to_string(fileIndex)+": ADDR_EXPECTED";
                    }
                    else{
                        parse3 = "Parse Error line "+to_string(fileLine)+" offset "+to_string(fileIndex-symbol.size())+": ADDR_EXPECTED";
                    }
                    handleErrors(parse3);
                }
                int val = getNextNumber();
                if(val==-1){
                    string parse1 = "Parse Error line "+to_string(fileLine)+" offset "+to_string(fileIndex-numOfDigits(val))+": NUM_EXPECTED";
                    handleErrors(parse1);
                }
                i++;
            }
            int nextMod = moduleAddress.at(currModule) + adrCount;
            moduleAddress.push_back(nextMod);
            currModule++;
            line=1;
        }
    }
}

int findInSymbolTable(string sym){
    for(int i=0;i<symbolTable.size();i++){
        if(symbolTable.at(i).symbolName.compare(sym)==0){
            symbolTable.at(i).used = true;
            return symbolTable.at(i).symbolAddress;
        }
    }
    return -1;
}

void passTwo(){
    int line = 1;
    moduleAddress.push_back(0);
    vector<string> usedSymbols;
    vector<int>usedSymbolCheck;
    while(currIndex<stringSize){
        if(line==1){
            int defCount = getNextNumber();
            int i=0;
            while(i<defCount){
                string symbol = getNextSymbol();
                int val = getNextNumber();
                i++;
            }
            line++;
        }
        else if(line==2){
            int symCount = getNextNumber();
            int i=0;
            while(i<symCount){
                string sym = getNextSymbol();
                usedSymbols.push_back(sym);
                i++;
            }
            line++;
        }
        else if(line==3){
            int adrCount = getNextNumber();
            int i=0;
            while(i<adrCount){
                string symbol = getNextSymbol();
                int val = getNextNumber();
                int opCode = val/1000;

                    int adr = val%1000;
                    if(symbol.compare("E")==0){
                        if(opCode>=10){
                                string err11 = "Error: Illegal opcode; treated as 9999";
                                addToMemoryTable(currAddress,9999,true,err11);
                            }
                        else if(adr>=usedSymbols.size()){
                            string err6 = "Error: External address exceeds length of uselist; treated as immediate";
                            addToMemoryTable(currAddress,val,true,err6);
                            currAddress++;
                        }
                        else{
                            string sym = usedSymbols.at(adr);
                            usedSymbolCheck.push_back(adr);
                            int adrFromST = findInSymbolTable(sym);
                            if(adrFromST==-1){
                                int actualAdr = 0 + (opCode*1000);
                                string err3 = "Error: " + sym + " is not defined; zero used";
                                addToMemoryTable(currAddress,actualAdr,true,err3);
                            }
                            else{
                                int actualAdr = adrFromST + (opCode*1000);
                                addToMemoryTable(currAddress,actualAdr,false,"No Error");
                            }
                            currAddress++;
                        }
                    }
                    else if(symbol.compare("R")==0){
                        int actualAdr = adr + moduleAddress.at(currModule);
                        if(actualAdr >= maxModuleAddress){
                            string err9 = "Error: Relative address exceeds module size; zero used";
                            actualAdr = moduleAddress.at(currModule) + (1000*opCode);
                            addToMemoryTable(currAddress,actualAdr,true,err9);
                        }
                        else{
                            actualAdr = actualAdr + (1000*opCode);
                            if(opCode>=10){
                                string err11 = "Error: Illegal opcode; treated as 9999";
                                addToMemoryTable(currAddress,9999,true,err11);
                            }
                            else{
                                addToMemoryTable(currAddress,actualAdr,false,"No Error");
                            }
                        }
                        currAddress++;
                    }
                    else if(symbol.compare("A")==0){
                        if(adr>512){
                            int actualAdr = 0 + (opCode*1000);
                            string err8 = "Error: Absolute address exceeds machine size; zero used";
                            addToMemoryTable(currAddress,actualAdr,true,err8);
                        }
                        else{
                            if(opCode>=10){
                                string err11 = "Error: Illegal opcode; treated as 9999";
                                addToMemoryTable(currAddress,9999,true,err11);
                            }
                            else{
                                addToMemoryTable(currAddress,val,false,"No Error");
                            }
                        }
                        currAddress++;
                    }
                    else if(symbol.compare("I")==0){
                        if(val >= 10000){
                            string err10 = "Error: Illegal immediate value; treated as 9999";
                            addToMemoryTable(currAddress,9999,true,err10);
                        }
                        else{
                            if(opCode>=10){
                                string err11 = "Error: Illegal opcode; treated as 9999";
                                addToMemoryTable(currAddress,9999,true,err11);
                            }
                            else{
                                addToMemoryTable(currAddress,val,false,"No Error");
                            }
                        }
                        currAddress++;
                    }
                    else{
                        handleErrors("Syntax error");
                    }

                i++;
            }
            for(int k=0;k<usedSymbols.size();k++){
                int found = 0;
                for(int j=0;j<usedSymbolCheck.size();j++){
                    if(usedSymbolCheck.at(j)==k){
                        found = 1;
                        break;
                    }
                }
                if(found==0){
                    string err7 = "Warning: Module " + to_string(currModule+1) + ": "
                                + usedSymbols.at(k) + " appeared in the uselist but was not actually used\n";
                    addToModuleError(2,currModule,err7);
                }
            }
            usedSymbols.clear();
            usedSymbolCheck.clear();
            int nextMod = moduleAddress.at(currModule) + adrCount;
            moduleAddress.push_back(nextMod);
            currModule++;
            line=1;
        }
    }
}

int main(int argc, char* argv[]){
    ifstream input_file(argv[1]);
    if ( currIndex<stringSize)
      cout<<"Could not open file\n";
    else {
        string temp;
        while(getline(input_file,temp)){
            str = str + temp;
            str = str + "\n";
        }
        //cout<<str<<"\n";
    }
    stringSize = str.size();
    passOne();
    maxModuleAddress = moduleAddress.at(currModule);
    currModule = 0;
    currIndex = 0;
    passTwo();

    cout<<"Symbol Table\n";
    int modulePrint1 = 0;
    for(int i=0;i<symbolTable.size();i++){
        if(symbolTable.at(i).module>modulePrint1){
            int flag=0;
            for(int j=0;j<moduleError.size();j++){
                if(moduleError.at(j).module==modulePrint1 && moduleError.at(j).pass==1){
                    cout<<moduleError.at(j).errorMessage;
                    modulePrint1++;
                    flag=1;
                    break;
                }
            }
            if(flag==0){
                modulePrint1++;
            }
        }
        cout<<symbolTable.at(i).symbolName<<"=";
        cout<<symbolTable.at(i).symbolAddress;
        if(symbolTable.at(i).error){
            cout<<" "<<symbolTable.at(i).errorMessage<<endl;
        }
        else{
            cout<<endl;
        }
        if(!(symbolTable.at(i).used)){
            string err4 = "Warning: Module "+to_string(symbolTable.at(i).module+1)+": "
                        + symbolTable.at(i).symbolName + " was defined but never used\n";
            errorMsgs = errorMsgs + err4;
        }
    }
    for(int j=0;j<moduleError.size();j++){
        if(moduleError.at(j).module==currModule-1 && moduleError.at(j).pass==1){
            cout<<moduleError.at(j).errorMessage;
            break;
        }
    }

    cout<<"\nMemory Map"<<endl;
    int modulePrint2 = 0;
    for(int i=0;i<memoryMap.size();i++){
        if(memoryMap.at(i).module>modulePrint2){
            int flag=0;
            for(int j=0;j<moduleError.size();j++){
                if(moduleError.at(j).module==modulePrint2 && moduleError.at(j).pass==2){
                    cout<<moduleError.at(j).errorMessage;
                    modulePrint2++;
                    flag=1;
                    break;
                }
            }
            if(flag==0){
                modulePrint2++;
            }
        }
        cout<<setfill('0')<<setw(3)<<memoryMap.at(i).adr1<<": ";
        cout<<setfill('0')<<setw(4)<<memoryMap.at(i).adr2;
        if(memoryMap.at(i).error){
            cout<<" "<<memoryMap.at(i).errorMessage<<endl;
        }
        else{
            cout<<endl;
        }
    }

    for(int j=0;j<moduleError.size();j++){
        if(moduleError.at(j).module==currModule-1 && moduleError.at(j).pass==2){
            cout<<moduleError.at(j).errorMessage;
            break;
        }
    }

    cout<<"\n"<<errorMsgs;

    return 0;
}
