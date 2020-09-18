#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<iomanip>
#include<unistd.h>
#include<queue>

using namespace std;

string str = "";
vector<int> randvals;
int ofs = 0;
int dotrace=1;
int maxprios = 4;
int quantum = 10000;
int sChosen=1;
int currentTime = 0;
int currPID = 0;
int totalNumOfProcesses=0;
int totalIO = 0;
int endTime;
bool CALL_SCHEDULER = false;
bool preempt = false;

//trace macro for verbose output testing
#define trace(fmt...)   do{if(dotrace>1){cout<<fmt;fflush(stdout);}}while(0)
//#define trace(fmt...)   do{ }while(0)

//transition enumeration type
typedef enum { STATE_FINISH, STATE_CREATED, STATE_READY, STATE_RUNNING, STATE_BLOCKED, TRANS_TO_READY, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_PREEMPT } process_state_t;

//Generates random number based on rfile
int myrandom(int burst) { 
    if(ofs==randvals.size()){
        ofs=0;
    }
    return 1 + (randvals[ofs++] % burst); 
}

//Holds information about each process
class Process {
	public:
        int AT, TC, CB, IB, pid;
		int rem, CB_carry;
        int dynamicPrio, staticPrio, lastBurst, lastBurstTime;
        int ready_time, FT, TT, IT, CW;
        process_state_t currState;
		Process(int a, int b, int c, int d){
			AT = a;
			TC = b;
			CB = c;
			IB = d;
			pid = currPID;
			currPID++;
			rem = TC;
            lastBurst = 0;
            CB_carry = 0;
            ready_time=AT;
            lastBurstTime = 0;
            IT=0;
            CW=0;
            FT=0;
            TT=0;
            dynamicPrio=0;
		}
};

//Processes are stored as doubly linked list in the queue format, thus each node represents a process
struct Node{
    Process* proc;
    Node* next;
    Node* prev;
    Node(Process *p){
        proc = p;
        next = nullptr;
        prev = nullptr;
    }
};

//Queue handles the push, pop operations in process queue based on scheduler requirements
class Queue {
    public:
        Node* front;
        Node* back;
        Queue(){
            front=nullptr;
            back=nullptr;
        }
        bool isEmpty(){
            if(front==nullptr || back==nullptr){
                return true;
            }
            else
                return false;
        }
        void addProcess(Process* p){
            if(isEmpty()){
                Node *temp0 = new Node(p);
                front = temp0;
                back = temp0;
            }
            else{
                Node *temp = new Node(p);
                back->prev=temp;
                temp->next=back;
                back=temp;
            }
        }
        Process* popProcess(){
            if(front==nullptr){
                return nullptr;
            }
            else{
                Process* p = front->proc;
                if(front->prev==nullptr){
                    front=nullptr;
                    back=nullptr;
                }
                else{
                    front=front->prev;
                    front->next=nullptr;
                }
                
                return p;
            }
        }

        Process* popProcessRem(){
            if(front==nullptr){
                return nullptr;
            }
            else{
                Node* temp = front;
                Node* min = front;
                while(temp!=nullptr){
                    if(min->proc->rem > temp->proc->rem){
                        min = temp;
                    }
                    temp=temp->prev;
                }
                if(min->prev == nullptr){
                    if(min->next != nullptr){
                        min->next->prev = nullptr;
                        back=min->next;
                    }
                    else{
                        front=nullptr;
                        back=nullptr;
                    }
                }
                else if(min->next == nullptr){
                    min->prev->next = nullptr;
                    front=min->prev;
                }
                else{
                    min->prev->next = min->next;
                    min->next->prev = min->prev;
                }
                return min->proc;
            }
        }

        Process* popLastProcess(){
            if(this->isEmpty()){
                return nullptr;
            }
            Node *temp = back;
            back=back->next;
            if(back!=nullptr)
                back->prev=nullptr;
            else{
                front=nullptr;
            }
            return temp->proc;
        }
        bool findInQueue(Process *p){
            Node* temp = front;
            while(temp!=nullptr){
                if(temp->proc->pid == p->pid){
                    return true;
                }
                else{
                    temp=temp->prev;
                }
            }
            return false;
        }
};

Queue inputQ;
Process* blocked = nullptr; //Process that is currently blocked
Process* running = nullptr; //Process that is currently running

//Holds information about an event
class Event {
    public:
        Process* evtProcess;
        int evtTimeStamp, evtDuration;
        process_state_t old_state, curr_state, transition;
        Event(){}
        Event(Process* p, int ts, process_state_t trans){
            evtProcess = p;
            evtTimeStamp = ts;
            transition = trans;
        }
};

//Simulates a layer that handles events by maintaining event queue; 
class DES {
    public:
        vector<Event*> eventQ;
        void addEvent(Event* e){
            int i=eventQ.size()-1;;
            while(i>=0 && e->evtTimeStamp < eventQ[i]->evtTimeStamp){
                i--;
            }
            eventQ.insert(eventQ.begin()+i+1,e);
        }
        Event* popEvent(){
            if(eventQ.empty()){
                Event* e = new Event(nullptr,-1,STATE_CREATED);
                return e;
            }
            else{
                Event* evt = eventQ[0];
                eventQ.erase(eventQ.begin());
                return evt;
            }
        }
        int nextEventTime(){
            if(eventQ.empty()){
                return -1;
            }
            else{
                return eventQ[0]->evtTimeStamp;
            }
        }

        //Handles preemption and returns true if preemption is needed
        bool runningProcessPreemption(int time){
            if(eventQ.empty()){
                return false;
            }
            else{
                for(int i=0;i<eventQ.size();i++){
                    Event* evt = eventQ[i];
                    if(evt->evtProcess->pid==running->pid){
                        if(evt->evtTimeStamp>time){
                            running->rem += evt->evtTimeStamp - time;
                            running->CB_carry = running->lastBurst - (time - running->lastBurstTime);
                            eventQ.erase(eventQ.begin()+i);
                            return true;
                        }
                        else{
                            return false;
                        }
                    }
                }
                return true;
            }
        }
};

//Object to simulate the input
DES* des;

//Base class of scheduler - functions are virtual
class Scheduler {
    public:
        vector<Queue*> priorityQ;
        virtual void add_process(Process *p)=0;
        virtual Process* get_next_process()=0;
        virtual void test_preempt(Process *p, int curtime )=0;
};

//FCFS scheduler implementing virtual functions from base class
class FCFS : public Scheduler {
    void add_process(Process *p){
        if(priorityQ.empty()){
            priorityQ.push_back(new Queue);
        }
        priorityQ[0]->addProcess(p);
    }
    Process* get_next_process(){
        if(priorityQ.empty()){
            return nullptr;
        }
        return priorityQ[0]->popProcess();
    }
    void test_preempt(Process *p, int curtime ){
        //do nothing
    }
};

//LCFS scheduler implementing virtual functions from base class
class LCFS : public Scheduler {
    void add_process(Process *p){
        if(priorityQ.empty()){
            priorityQ.push_back(new Queue);
        }
        priorityQ[0]->addProcess(p);
    }
    Process* get_next_process(){
        if(priorityQ.empty()){
            return nullptr;
        }
        return priorityQ[0]->popLastProcess();
    }
    void test_preempt(Process *p, int curtime ){
        //do nothing
    }
};

//SRTF scheduler implementing virtual functions from base class
class SRTF : public Scheduler {
    void add_process(Process *p){
        if(priorityQ.empty()){
            priorityQ.push_back(new Queue);
        }
        priorityQ[0]->addProcess(p);
        //cout<<priorityQ[0]->front->proc->pid<<" : "<<currentTime<<endl;
    }
    Process* get_next_process(){
        if(priorityQ.empty()){
            return nullptr;
        }
        return priorityQ[0]->popProcessRem();
    }
    void test_preempt(Process *p, int curtime ){
        //do nothing
    }
};

//Round Robin scheduler implementing virtual functions from base class
class RR : public Scheduler {
    void add_process(Process *p){
        if(priorityQ.empty()){
            priorityQ.push_back(new Queue);
        }
        priorityQ[0]->addProcess(p);
    }
    Process* get_next_process(){
        if(priorityQ.empty()){
            return nullptr;
        }
        return priorityQ[0]->popProcess();
    }
    void test_preempt(Process *p, int curtime ){
        //do nothing
    }
};

//Priority scheduler implementing virtual functions from base class
class PRIO : public Scheduler {
    vector<Queue*> activeQ;     //Active priority queue
    vector<Queue*> expiredQ;    //Expired priority queue
    void add_process(Process *p){
        if(activeQ.empty()&&expiredQ.empty()){
            for(int i=0;i<maxprios;i++){        //Initialize list of queue
                activeQ.push_back(new Queue);
                expiredQ.push_back(new Queue);
            }
        }
        if(preempt){    //When transitioning from preempt to ready, dynamic priority is decremented
            if(p->dynamicPrio<=0){      //if dynamic priority is below min it is added to expired queue with reset priority
                p->dynamicPrio = p->staticPrio-1;
                expiredQ[p->dynamicPrio]->addProcess(p);
            }
            else{
                p->dynamicPrio--;
                activeQ[p->dynamicPrio]->addProcess(p);
            }
            preempt=false;
        }
        else{           //When ready process is transitioning ro run
            activeQ[p->dynamicPrio]->addProcess(p);
        }
    }
    Process* get_next_process(){
        if(activeQ.empty()||expiredQ.empty()){
            return nullptr;
        }
        for(int i=1;i<=maxprios;i++){
            if(!(activeQ[maxprios-i]->isEmpty())){
                return activeQ[maxprios-i]->popProcess();
            }
        }
        //If no process found in active queues, active and expired queues are swapped
        priorityQ = activeQ;
        activeQ = expiredQ;
        expiredQ = priorityQ;
        for(int i=1;i<=maxprios;i++){
            if(!(activeQ[maxprios-i]->isEmpty())){
                return activeQ[maxprios-i]->popProcess();
            }
        }
        return nullptr;
    }
    void test_preempt(Process *p, int curtime ){
        //do nothing
    }
};

//PREPRIO scheduler implementing virtual functions from base class
//Same as Priority scheduler, except implimentation of test_preempt function
class PREPRIO : public Scheduler {
    vector<Queue*> activeQ;
    vector<Queue*> expiredQ;
    void add_process(Process *p){
        if(activeQ.empty()&&expiredQ.empty()){
            for(int i=0;i<maxprios;i++){
                activeQ.push_back(new Queue);
                expiredQ.push_back(new Queue);
            }
        }
        if(preempt){
            if(p->dynamicPrio<=0){
                p->dynamicPrio = p->staticPrio-1;
                expiredQ[p->dynamicPrio]->addProcess(p);
            }
            else{
                p->dynamicPrio--;
                activeQ[p->dynamicPrio]->addProcess(p);
            }
            preempt=false;
        }
        else{
            activeQ[p->dynamicPrio]->addProcess(p);
        }
    }
    Process* get_next_process(){
        if(activeQ.empty()||expiredQ.empty()){
            return nullptr;
        }
        for(int i=1;i<=maxprios;i++){
            if(!(activeQ[maxprios-i]->isEmpty())){
                return activeQ[maxprios-i]->popProcess();
            }
        }
        priorityQ = activeQ;
        activeQ = expiredQ;
        expiredQ = priorityQ;
        for(int i=1;i<=maxprios;i++){
            if(!(activeQ[maxprios-i]->isEmpty())){
                return activeQ[maxprios-i]->popProcess();
            }
        }
        return nullptr;
    }
    bool checkIfExpired(Process* run){          //Checks if the process is in expired queue
        for(int i=1;i<maxprios;i++){
            if(expiredQ[maxprios-i]->findInQueue(run)){
                return true;
            }
        }
        return false;
    }

    //Checks of the ready process should preempt the current process
    void test_preempt(Process *p, int curtime ){
        if(running!=nullptr){
            if(p->dynamicPrio>running->dynamicPrio || checkIfExpired(running)){
                //If priority check suggests premeption, we check if next event is at current time or not
                //Accoridingly the event is removed and new event added. The below functions removes 
                //the next event of running process and returns true if premption required. Then a new
                //event is created. Else, returns false and preemption is not required as running process 
                //is getting blocked at the current time even without preemption.
                if(des->runningProcessPreemption(curtime)){
                    Event* e = new Event(running,curtime,TRANS_TO_PREEMPT);
                    des->addEvent(e);
                }
            }
        }
    }
};

//Creates input process queue from file input
void takeInputs(string p){
    int i=0;
    int x=0;
    int count=0;
    int a=0;
    int b=0;
    int c=0;
    int d=0;
    
    while(i<p.size()){
        if(p[i]=='\n'){
            Process *temp = new Process(a,b,c,d);
            temp->staticPrio = myrandom(maxprios);
            temp->dynamicPrio=temp->staticPrio-1;
            string p1=to_string(temp->pid)+" : "+to_string(temp->AT)+" "+to_string(temp->TC)+" "+to_string(temp->CB)+" "+to_string(temp->IB);
            trace(p1);
            inputQ.addProcess(temp);
            x++;
            count=0;
            i++;
        }
        else if(isdigit(p[i])){
            int num=p[i]-48;
            i++;
            while(i<p.size() && isdigit(p[i])){
                num=num*10+(p[i]-48);
                i++;
            }
            switch (count){
                case 0:
                    a=num;
                    break;
                case 1:
                    b=num;
                    break;
                case 2:
                    c=num;
                    break;
                case 3:
                    d=num;
                    break;
                default:
                    break;
            }
            count++;
        }
        else{
            i++;
        }
    }
}

//Initializes the simulation by adding all processes to event queue, based on arrival times
DES* initializeDES(){
    DES* des = new DES;
    Node* temp = inputQ.front;
    while(temp!=nullptr){
        Process* p = temp->proc;
        Event* e = new Event(p, p->AT, TRANS_TO_READY);
        e->old_state = STATE_CREATED;
        e->curr_state = STATE_READY;
        e->evtDuration = 0;
        des->addEvent(e);
        temp = temp->prev;
    }
    return des;
}

//Simulates the processes transitioning to different states, based on the scheduler chosen
void simulation(Scheduler* sched){
    des = initializeDES();
    int burst;
    int io_end;
    int ioBurst;
    for(Event* evt = des->popEvent();evt->evtTimeStamp!=-1&&evt->evtProcess!=nullptr;evt=des->popEvent()){
        Process* p = evt->evtProcess;
        currentTime = evt->evtTimeStamp;
        //cout<<p->pid<<" : "<<p->rem<<" "<<currentTime<<" "<<evt->transition<<" "<<p->dynamicPrio<<endl;
        Event* e;
        switch(evt->transition){
            case TRANS_TO_READY:
                if(blocked == p){
                    blocked = nullptr;
                }
                p->ready_time=currentTime;     
                sched->add_process(p);
                sched->test_preempt(p,currentTime); 
                CALL_SCHEDULER = true;
                break;
            case TRANS_TO_RUN:
                running = p;
                if(p->CB_carry>0){
                    burst = p->CB_carry;
                }
                else{
                    burst = myrandom(p->CB);
                }
                if(burst>=p->rem){
                    burst = p->rem;
                }
                p->lastBurst=burst;
                p->lastBurstTime = currentTime;
                p->CW += currentTime - p->ready_time;
                if(burst>quantum){
                    p->rem=p->rem - quantum;
                    p->CB_carry = burst - quantum;
                    e = new Event(p,currentTime+quantum,TRANS_TO_PREEMPT);
                    e->evtDuration = quantum;
                    e->old_state = STATE_RUNNING;
                    des->addEvent(e);
                }
                else if(burst==p->rem){
                    p->rem = p->rem - burst;
                    e = new Event(p,currentTime+burst,STATE_FINISH);
                    e->evtDuration = burst;
                    e->old_state = STATE_RUNNING;
                    des->addEvent(e);
                }
                else if(burst<p->rem){
                    p->rem = p->rem - burst;
                    p->CB_carry = -1;
                    e = new Event(p,currentTime+burst,TRANS_TO_BLOCK);
                    e->evtDuration = burst;
                    e->old_state = STATE_RUNNING;
                    des->addEvent(e);
                }
                break;
            case TRANS_TO_BLOCK:
                running = nullptr;
                ioBurst = myrandom(p->IB);
                p->IT = p->IT + ioBurst;
                if(blocked==nullptr){
                    totalIO = totalIO+ioBurst;
                    io_end = currentTime + ioBurst;
                    blocked = p;
                }
                else{
                    if(ioBurst + currentTime > io_end){
                        totalIO = totalIO + ioBurst + currentTime - io_end;
                        io_end = ioBurst + currentTime;
                        blocked = p;
                    }
                }
                e = new Event(p,currentTime+ioBurst,TRANS_TO_READY);
                e->evtDuration = ioBurst;
                e->old_state = STATE_BLOCKED;
                des->addEvent(e);
                CALL_SCHEDULER = true;
                p->dynamicPrio=p->staticPrio-1;
                break;
            case TRANS_TO_PREEMPT:
                preempt = true;
                sched->add_process(p);
                running=nullptr;
                p->ready_time = currentTime;
                CALL_SCHEDULER = true;
                break;
            case STATE_FINISH:
                p->FT=currentTime;
                running = nullptr;
                endTime=currentTime;
                CALL_SCHEDULER = true;
                break;
            default:
                break;
        }
        
        if(CALL_SCHEDULER){
            if(des->nextEventTime()==currentTime){
                continue;
            }
            CALL_SCHEDULER = false;
            if(running == nullptr){
                running = sched->get_next_process();
                if(running == nullptr){
                    continue;
                }
                Event* e = new Event(running,currentTime,TRANS_TO_RUN);
                e->evtDuration = 0;
                e->old_state = STATE_READY;
                des->addEvent(e);
                running = nullptr;
            }
        }
    }
}

//Calculates the averages and prints output in the required format
void printOutput(string name){
    cout<<name<<endl;
    int total_cpu_time=0;
    double average_turnaround_time = 0;
    double average_wait_time = 0;
    Node* temp = inputQ.front;
    while(temp!=nullptr){
        Process* p = temp->proc;
        total_cpu_time += p->TC;
        p->TT=p->FT - p->AT;
        average_turnaround_time += p->TT;
        average_wait_time += p->CW;
        cout<<setfill('0')<<setw(4)<<p->pid<<": ";
        cout<<setfill(' ')<<setw(4)<<p->AT<<" ";
        cout<<setfill(' ')<<setw(4)<<p->TC<<" ";
        cout<<setfill(' ')<<setw(4)<<p->CB<<" ";
        cout<<setfill(' ')<<setw(4)<<p->IB<<" ";
        cout<<setfill(' ')<<setw(1)<<p->staticPrio<<" | ";
        cout<<setfill(' ')<<setw(5)<<p->FT<<" ";
        cout<<setfill(' ')<<setw(5)<<p->TT<<" ";
        cout<<setfill(' ')<<setw(5)<<p->IT<<" ";
        cout<<setfill(' ')<<setw(5)<<p->CW<<endl;
        temp=temp->prev;
    }
    double cpu_utilization = (total_cpu_time/(double)endTime)*100.0;
    double io_utilization = (totalIO/(double)endTime)*100.0;
    average_turnaround_time /= (double)totalNumOfProcesses;
    average_wait_time /= (double)totalNumOfProcesses;
    double throughtput = (totalNumOfProcesses/(double)endTime)*100.0;

    cout<<"SUM: ";
    cout<<endTime<<" ";
    cout<<fixed<<setprecision(2)<<cpu_utilization<<" ";
    cout<<fixed<<setprecision(2)<<io_utilization<<" ";
    cout<<fixed<<setprecision(2)<<average_turnaround_time<<" ";
    cout<<fixed<<setprecision(2)<<average_wait_time<<" ";
    cout<<fixed<<setprecision(3)<<throughtput<<endl;
    
}

int main(int argc, char* argv[]){
    string schedulerName="";
    Scheduler* sched;
    //Input from file
    ifstream input_file(argv[argc-2]);
    if ( argc<3)
      cout<<"Could not open file\n";
    else {
        string temp;
        while(getline(input_file,temp)){
            str = str + temp;
            str = str + "\n";
            totalNumOfProcesses++;
        }
    }
    //Input of random numbers from file
    ifstream random_file(argv[argc-1]);
    int next = 0;
    random_file>>next;
    while (random_file >> next) {
        randvals.push_back(next);
    }
    int vflag = 0;
    int tflag = 0;
    int eflag = 0;
    string svalue="";
    int index;
    int c;

    opterr = 0;
    //getting options from command line inputs
    while ((c = getopt (argc, argv, "vVtTeEs:S:")) != -1)
        switch (c){
            case 'v':
                vflag = 1;
                break;
            case 'V':
                vflag = 1;
                break;
            case 't':
                tflag = 1;
                break;
            case 'T':
                tflag = 1;
                break;
            case 'e':
                eflag = 1;
                break;
            case 'E':
                eflag = 1;
                break;
            case 's':
                svalue = svalue + optarg;
                break;
            case 'S':
                svalue = svalue + optarg;
                break;
            case '?':
                if (optopt == 'c')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
                return 1;
            default:
            abort ();
        }
    if(vflag==1){
        dotrace=2;
    }
    trace(svalue);
    trace("\n");
    
    //extracting scheduler specification from command line input option s
    //Handling both lower and upper case characters
    for(int i=0;i<svalue.size();i++){
        int numr=0;
        int nump=0;
        int nume=0;
        if(isalpha(svalue[i])){
            switch (svalue[i]){
                case 'F':
                    sChosen=1;
                    break;
                case 'f':
                    sChosen=1;
                    break;
                case 'L':
                    sChosen=2;
                    break;
                case 'l':
                    sChosen=2;
                    break;
                case 'S':
                    sChosen=3;
                    break;
                case 's':
                    sChosen=3;
                    break;
                case 'R':
                    sChosen=4;
                    while((i+1)<svalue.size() && isdigit(svalue[i+1])){
                        i++;
                        numr = numr * 10 + (svalue[i] - 48);
                    }
                    quantum = numr;
                    break;
                case 'r':
                    sChosen=4;
                    while((i+1)<svalue.size() && isdigit(svalue[i+1])){
                        i++;
                        numr = numr * 10 + (svalue[i] - 48);
                    }
                    quantum = numr;
                    break;
                case 'P':
                    sChosen=5;
                    while((i+1)<svalue.size() && isdigit(svalue[i+1])){
                        i++;
                        nump = nump * 10 + (svalue[i] - 48);
                    }
                    quantum = nump;
                    i++;
                    if(svalue[i]==':'){
                        int priop=0;
                        while((i+1)<svalue.size() && isdigit(svalue[i+1])){
                            i++;
                            priop = priop * 10 + (svalue[i] - 48);
                        }
                        maxprios = priop;
                    }
                    break;
                case 'p':
                    sChosen=5;
                    while((i+1)<svalue.size() && isdigit(svalue[i+1])){
                        i++;
                        nump = nump * 10 + (svalue[i] - 48);
                    }
                    quantum = nump;
                    i++;
                    if(svalue[i]==':'){
                        int priop=0;
                        while((i+1)<svalue.size() && isdigit(svalue[i+1])){
                            i++;
                            priop = priop * 10 + (svalue[i] - 48);
                        }
                        maxprios = priop;
                    }
                    break;
                case 'E':
                    sChosen=6;
                    while((i+1)<svalue.size() && isdigit(svalue[i+1])){
                        i++;
                        nume = nume * 10 + (svalue[i] - 48);
                    }
                    quantum = nume;
                    i++;
                    if(svalue[i]==':'){
                        int prioe=0;
                        while((i+1)<svalue.size() && isdigit(svalue[i+1])){
                            i++;
                            prioe = prioe * 10 + (svalue[i] - 48);
                        }
                        maxprios = prioe;
                    }
                    break;
                case 'e':
                    sChosen=6;
                    while((i+1)<svalue.size() && isdigit(svalue[i+1])){
                        i++;
                        nume = nume * 10 + (svalue[i] - 48);
                    }
                    quantum = nume;
                    i++;
                    if(svalue[i]==':'){
                        int prioe=0;
                        while((i+1)<svalue.size() && isdigit(svalue[i+1])){
                            i++;
                            prioe = prioe * 10 + (svalue[i] - 48);
                        }
                        maxprios = prioe;
                    }
                    break;
                default:
                    trace("Invalid Input");
                    trace("\n");
            }
        }
    }
    
    //verbose output to check if input is interpretted correctly
    string tracestr="";
        tracestr=tracestr+"Scheduler: "+to_string(sChosen)+"\nQuantum: "+to_string(quantum)+"\nMaxPrios: "+
                to_string(maxprios)+"\nvflag: "+to_string(vflag)+"\ntflag: "+to_string(tflag)+"\neflag: "+to_string(eflag)+"\n";
        trace(tracestr);
        
    //Based on inputs, scheduler object is referenced to the object of chosen scheduler
    switch(sChosen){
        case 1:
            sched = new FCFS();
            schedulerName = "FCFS";
            break;
        case 2:
            sched = new LCFS();
            schedulerName = "LCFS";
            break;
        case 3:
            sched = new SRTF();
            schedulerName="SRTF";
            break;
        case 4:
            sched = new RR();
            schedulerName = "RR "+to_string(quantum);
            break;
        case 5:
            sched = new PRIO();
            schedulerName = "PRIO "+to_string(quantum);
            break;
        case 6:
            sched = new PREPRIO();
            schedulerName = "PREPRIO "+to_string(quantum);
            break;
        default:
            sched = new FCFS();
            schedulerName = "FCFS";
            break;
    }
    takeInputs(str);    //Created process queue from input
    simulation(sched);  //Simulates the scheduling of processes
    printOutput(schedulerName); //Prints output in required format
    return 0;
}
