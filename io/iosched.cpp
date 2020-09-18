#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<iomanip>
#include<unistd.h>
#include<cmath>

using namespace std;

//Global variables used throughout the program
string str = "";
string scheduler_choice = "";
int size_str = 0;
int current_index = 0;
bool v_flag = false;
bool q_flag = false;
bool f_flag = false;
int current_time = 0;
int input_request_index = 0;
bool io_free = true;
int current_track = 0;
int total_requests_completed = 0;
int total_movement = 0;
int look_direction = 0;
int time_delta = 0;

//Structure to hold request details
class Request {
    public:
        int arrival_time;
        int track;
        int start_time;
        int end_time;

        Request(int at, int trk){
            arrival_time = at;
            track = trk;
        }
};

//Stores IO requests from input file
vector<Request*> io_requests;

//Doubly linked list structure to implement a queue of requests
struct Node{
    Request* req;
    Node* next;
    Node* prev;
    Node(Request *r){
        req = r;
        next = nullptr;
        prev = nullptr;
    }
};

//Queue with push and pop funtionality tailor-made for different algorithms
class Queue {
    public:
    Node *front;
    Node *last;
    Node *curr;
    Node *next;
    Node *prev;
    
    Queue(){
        front = nullptr;
        last = nullptr;
        curr = nullptr;
        next = nullptr;
        prev = nullptr;
    }
    
    bool is_empty(){
        if(!front || !last){
            return true;
        }
        else{
            return false;
        }
    }

    //push as in a normal queue's enqueue operation
    void push(Request *temp){
        if(!front || !last){
            curr = new Node(temp);
            front = curr;
            last = curr;
        }
        else{
            curr = new Node(temp);
            front->prev = curr;
            curr->next = front;
            front = curr;
        }
    }
    
    //pop as in normal queue's dequeue operation and returs the request in FIFO order
    Request* pop(){
        if(is_empty()){
            return nullptr;
        }
        else{
            Request *r = last->req;
            last = last->prev;
            if(!last){
                front = nullptr;
            }
            else{
                last->next = nullptr;
            }
            return r;
        }
    }
    
    //Returns request with shortest seek time in the queue
    Request* pop_with_shortest_seek_time(){
        if(is_empty()){
            return nullptr;
        }
        else{
            Request *r = last->req;
            Node *temp = last;
            Node *chosen = temp;
            while(temp){
                if(abs(current_track - temp->req->track) < abs(current_track - r->track)){
                    r = temp->req;
                    chosen = temp;
                }
                temp = temp->prev;
            }
            if(chosen->prev && !chosen->next){
                temp = chosen->prev;
                temp->next = chosen->next;
                last = temp;
            }
            else if(!chosen->prev && chosen->next){
                temp = chosen->next;
                temp->prev = chosen->prev;
                front = temp;
            }
            else if(chosen->prev && chosen->next){
                temp = chosen->prev;
                temp->next = chosen->next;
                chosen->next->prev = temp;
            }
            else{
                front = nullptr;
                last = nullptr;
            }
            return r;
        }
    }
    
    //Returns the request with closest track on same direction of movement of seeker
    Request* pop_for_look(){
        if(!front || !last){
            return nullptr;
        }
        else{
            Request *r = last->req;
            Node *temp = last;
            Node *chosen = temp;
            while(temp){
                if(look_direction == 0){
                    //If direction is not specified, the closest track request is chosen
                    if(abs(current_track - temp->req->track) < abs(current_track - r->track)){
                        r = temp->req;
                        chosen = temp;
                    }
                }
                else if(look_direction == -1){
                    //If direction is -ve, we search the closest track in that direction
                    if((current_track - temp->req->track) >= 0){
                        if(current_track - r->track >= 0){
                            if(current_track - r->track > current_track - temp->req->track){
                                r = temp->req;
                                chosen = temp;
                            }
                        }
                        else{
                            r = temp->req;
                            chosen = temp;
                        }
                    }
                }
                else{
                    //If direction is +ve, we search the closest track in that direction
                    if((temp->req->track - current_track) >= 0){
                        if(r->track - current_track >= 0){
                            if(r->track - current_track > temp->req->track - current_track){
                                r = temp->req;
                                chosen = temp;
                            }
                        }
                        else{
                            r = temp->req;
                            chosen = temp;
                        }
                    }
                }
                temp = temp->prev;
            }
            //If no request found in the said direction, direction is changed
            //and again searched with the same popping mechanism
            if(r == last->req){
                if(look_direction == -1){
                    if(current_track < r->track){
                        look_direction = 1;
                        return pop_for_look();
                    }
                }
                else if(look_direction == 1){
                    if(current_track > r->track){
                        look_direction = -1;
                        return pop_for_look();
                    }
                }
            }
            if(chosen->prev && !chosen->next){
                temp = chosen->prev;
                temp->next = chosen->next;
                last = temp;
            }
            else if(!chosen->prev && chosen->next){
                temp = chosen->next;
                temp->prev = chosen->prev;
                front = temp;
            }
            else if(chosen->prev && chosen->next){
                temp = chosen->prev;
                temp->next = chosen->next;
                chosen->next->prev = temp;
            }
            else{
                front = nullptr;
                last = nullptr;
            }
            //Based on the chosen request, the direction of seeker movement is set
            if(r->track < current_track){
                look_direction = -1;
            }
            else if(r->track > current_track){
                look_direction = 1;
            }
            return r;
        }
    }

    //Returns the request with closest track on +ve search direction
    //If no more requests in that direction are found, the current track goes back to min track in queue
    Request* pop_for_clook(){
        if(!front || !last){
            return nullptr;
        }
        else{
            Request *r = last->req;
            Node *temp = last;
            Node *chosen = temp;
            int min_track_so_far = 2147483640;
            while(temp){
                if(temp->req->track < min_track_so_far){
                    min_track_so_far = temp->req->track;
                }
                if((temp->req->track - current_track) >= 0){
                    if(r->track - current_track >= 0){
                        if(r->track - current_track > temp->req->track - current_track){
                            r = temp->req;
                            chosen = temp;
                        }
                    }
                    else{
                        r = temp->req;
                        chosen = temp;
                    }
                }
                temp = temp->prev;
            }
            if(r == last->req){
                if(current_track > r->track){
                    //Simulating current track to move to min track found in queue
                    //To leave the simulation part untouched, instead of decreasing track to reach min track,
                    //current track is given a -ve value, which can be thought of as moving in circle in the disk
                    //this is done to simulate same time requirement to get to the min track
                    current_track = min_track_so_far*2 - current_track;
                    return pop_for_clook();
                }
            }
            if(chosen->prev && !chosen->next){
                temp = chosen->prev;
                temp->next = chosen->next;
                last = temp;
            }
            else if(!chosen->prev && chosen->next){
                temp = chosen->next;
                temp->prev = chosen->prev;
                front = temp;
            }
            else if(chosen->prev && chosen->next){
                temp = chosen->prev;
                temp->next = chosen->next;
                chosen->next->prev = temp;
            }
            else{
                front = nullptr;
                last = nullptr;
            }
            return r;
        }
    }
};

//Base class for IO schedulers
class Strategy {
    public:
        Queue *request_queue;
        virtual Request* get_next_request() = 0;
        virtual void add_request(Request *r) = 0;
};

//FIFO algorithm to find the next request
class FIFO : public Strategy {
    public:
        FIFO(){
            request_queue = new Queue();
        }

        Request* get_next_request(){
            return request_queue->pop();
        }

        void add_request(Request *r){
            request_queue->push(r);
        }
};

//SSTF algorithm to find the next request
class SSTF : public Strategy {
    public:
        SSTF(){
            request_queue = new Queue();
        }

        Request* get_next_request(){
            return request_queue->pop_with_shortest_seek_time();
        }

        void add_request(Request *r){
            request_queue->push(r);
        }
};

//LOOK algorithm to find the next request
class LOOK : public Strategy {
    public:
        LOOK(){
            request_queue = new Queue();
        }

        Request* get_next_request(){
            return request_queue->pop_for_look();
        }

        void add_request(Request *r){
            request_queue->push(r);
        }
};

//CLOOK algorithm to find the next request
class CLOOK : public Strategy {
    public:
        CLOOK(){
            request_queue = new Queue();
        }

        Request* get_next_request(){
            return request_queue->pop_for_clook();
        }

        void add_request(Request *r){
            request_queue->push(r);
        }
};

//FLOOK algorithm to find the next request
class FLOOK : public Strategy {
    public:
        //Queue from where closest requests are searched aligned with direction and returned
        Queue *active_queue;
        //Queue where requests are added when they arrive
        Queue *add_queue;

        FLOOK(){
            active_queue = new Queue();
            add_queue = new Queue();
        }

        Request* get_next_request(){
            //Swap queues if active_queue is empty
            if(active_queue->is_empty()){
                Queue *temp = active_queue;
                active_queue = add_queue;
                add_queue = temp;
            }

            //If active queue is not empty, return request from active_queue based on LOOK algorithm
            if(!active_queue->is_empty()){
                return active_queue->pop_for_look();
            }

            //Return null if no requests in either queue
            return nullptr;
        }

        //Push request in add_queue
        void add_request(Request *r){
            add_queue->push(r);
        }
};

//Base class object to refer the chosen algorithm
Strategy *algo;
//Holds pointer to current request that is being processed
Request *current_request;

//using input to choose scheduling algorithm
void decrypt_input(){
    switch(scheduler_choice[0]){
    case 'i':
        algo = new FIFO();
        break;
    case 'j':
        algo = new SSTF();
        break;
    case 's':
        algo = new LOOK();
        break;
    case 'c':
        algo = new CLOOK();
        break;
    case 'f':
        algo = new FLOOK();
        break;
    default:
        algo = new FIFO();
        break;
    }
}

//Reads requests from input file
void read_input(){
    while(current_index<size_str){
        while(current_index<size_str&&(!isdigit(str[current_index]))){current_index++;}
        int arrive = 0;
        int track = 0;
        bool flag = false;
        while(current_index<size_str&&isdigit(str[current_index])){
            arrive = arrive*10 + (str[current_index] - 48);
            current_index++;
            flag = true;
        }
        while(current_index<size_str&&(!isdigit(str[current_index]))){current_index++;}
        while(current_index<size_str&&isdigit(str[current_index])){
            track = track*10 + (str[current_index] - 48);
            current_index++;
            flag = true;
        }
        if(flag){
            Request *temp = new Request(arrive, track);
            io_requests.push_back(temp);
        }
    }
}

//Prints input to test if the input was taken correctly
//Only for testing purpose - Not required in grading
void print_input(){
    cout<<"Number of requests = "<<io_requests.size()<<endl;
    for(int i=0;i<io_requests.size();i++){
        cout<<io_requests[i]->arrival_time<<" "<<io_requests[i]->track<<endl;
    }
}

//Simulates increment in time and adding requests to schedulers if arrival time of any request is reached
//The scheduler returns the appropriate request to process. The statistics are also summarized
void simulate(){
    while(true){
        //Exit simulation whell all requests are processed
        if(total_requests_completed == io_requests.size()){
            break;
        }

        //If a request arrives as current time, it is added in request queue of scheduler
        if(input_request_index < io_requests.size() && io_requests[input_request_index]->arrival_time == current_time){
            algo->add_request(io_requests[input_request_index]);
            input_request_index++;
        }

        //If there are no requests currently being processed, next request is asked from scheduler
        if(!current_request){
            current_request = algo->get_next_request();
            //If scheduler request queue is empty wait until some other request arrives
            if(!current_request){
                current_time++;
                continue;
            }
            current_request->start_time = current_time;
        }

        //Reached the required track as per the request, the request is completed
        if(current_track == current_request->track){
            current_request->end_time = current_time;
            current_request = nullptr;
            total_requests_completed++;
            continue;
        }

        //Based on the direction of next track to be seeked, current track is incremented or decremented
        if(current_request && current_request->track > current_track){
            current_track++;
            total_movement++;
        }
        else if(current_request && current_request->track < current_track){
            current_track--;
            total_movement++;
        }
        //Time incremented if there is a request in process and not completed yet
        current_time++;
    }
}

//Prints output in the required format
void print_output(){
    double average_turnaround = 0.0;
    double average_waittime = 0.0;
    int max_wait_time = 0;
    for(int i=0;i<io_requests.size();i++){
        cout<<setfill(' ')<<setw(5)<<i<<": ";
        cout<<setfill(' ')<<setw(5)<<io_requests[i]->arrival_time<<" ";
        cout<<setfill(' ')<<setw(5)<<io_requests[i]->start_time<<" ";
        cout<<setfill(' ')<<setw(5)<<io_requests[i]->end_time<<endl;
        if(io_requests[i]->start_time - io_requests[i]->arrival_time > max_wait_time){
            max_wait_time = io_requests[i]->start_time - io_requests[i]->arrival_time;
        }
        average_turnaround += (double)(io_requests[i]->end_time - io_requests[i]->arrival_time);
        average_waittime += (double)(io_requests[i]->start_time - io_requests[i]->arrival_time);
    }

    average_turnaround /= io_requests.size();
    average_waittime /= io_requests.size();
    cout<<"SUM: "<<current_time<<" "<<total_movement<<" ";
    cout<<fixed<<setprecision(2)<<average_turnaround<<" ";
    cout<<fixed<<setprecision(2)<<average_waittime<<" ";
    cout<<max_wait_time<<endl;
}

//Handles input, options, calls initializers and simulator
int main(int argc, char* argv[]){

    //Input from file
    ifstream input_file(argv[argc-1]);
    if ( argc<2)
      cout<<"Could not open file\n";
    else {
        string temp;
        while(getline(input_file,temp)){
            if(temp[0]!='#'){
                str += temp;
                str += "\n";
            }
        }
        size_str = str.size();
    }

    int c;
    //getting options from command line inputs
    while ((c = getopt (argc, argv, "s:v:q:f:")) != -1)
        switch (c){
            case 's':
                scheduler_choice = scheduler_choice + optarg;
                break;
            case 'v':
                f_flag = true;
                break;
            case 'q':
                q_flag = true;
                break;
            case 'f':
                f_flag = true;
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
    
    //Based on the chosen options, scheduler is chosen
    decrypt_input();

    //Input from file is read and stored in relavent variables
    read_input();

    //Only for testing
    //print_input();
    
    //Simulate processing of io requests
    simulate();

    //Calculating costs and printing statistics and useful information in required format
    print_output();

    return 0;
}
