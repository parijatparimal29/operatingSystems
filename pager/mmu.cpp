#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<iomanip>
#include<unistd.h>
#include<queue>

using namespace std;

vector<int> randvals;
int ofs = 0;
string str = "";
int num_of_frames = 0;
char algo = 'f';
bool print_O = false;
bool print_P = false;
bool print_F = false;
bool print_S = false;
bool x_flag = false;
bool y_flag = false;
bool f_flag = false;
bool a_flag = false;
string input_OPFS = "";
string input_num_of_frames = "";
string input_algo = "";
const int MAX_VPAGE_SIZE = 64;
const int FRAME_TABLE_SIZE = 128;
int num_of_processes = 0;
int current_index = 0;
int size_str = 0;
int instruction_counter = 0;
int vpage = -1;
char operation = 'x';

//get random number using rfile
int myrandom(int burst) { 
    if(ofs==randvals.size()){
        ofs=0;
    }
    return 1 + (randvals[ofs++] % burst); 
}

//stores and updates count of operations for each process
class Pstats{
    public:
        long context_switch_count, read_count, write_count;
        long map_count, unmap_count, in_count, out_count, fin_count, fout_count;
        long zero_count, segv_count, segprot_count, exit_count;
        Pstats(){
            context_switch_count = 0;
            read_count = 0;
            write_count = 0;
            map_count = 0;
            unmap_count = 0;
            in_count = 0;
            out_count = 0;
            fin_count = 0;
            fout_count = 0;
            zero_count = 0;
            segv_count = 0;
            segprot_count = 0;
            exit_count = 0;
        }
};

//Used to store the stats for each process
vector<Pstats*> pstats;

//Provides data structure to store VMAs in usable way
class VMA{
    public:
        int starting_vpage;
        int ending_vpage;
        int write_protected;
        int file_mapped;
};

//32-bit PTE data structure with read and write implementation
class PTE{
    public:
        unsigned int valid:1;
	    unsigned int reference:1;
	    unsigned int modified:1;
	    unsigned int write_protect:1;
        unsigned int paged_out:1;
	    unsigned int num_of_physical_frames:7;
        unsigned int file_mapped:1;
        unsigned int virutal_address:6;
        unsigned int extra:13;

        PTE(){
            valid = 0;
            reference = 0;
            modified = 0;
            write_protect = 0;
            paged_out = 0;
            num_of_physical_frames = 0;
            file_mapped = 0;
            virutal_address = 0;
            extra = 0;
        }

        void read(int pid){
            pstats[pid]->read_count++;
            reference = 1;
        }

        void write(int pid){
            pstats[pid]->write_count++;
            modified = 1;
        }

        void reset(){
            valid = 0;
            reference = 0;
            modified = 0;
        }
};

//Data structure for each process containing its pid, VMAs and page table
class Process{
    public:
        vector<VMA*> VMA_list;
        vector<PTE*> page_table;
        int pid;
        
        Process(int id){
            pid = id;
        }
};

//Data structure to hold values useful in frames for different algorithms
class Frame{
    public:
        int frame_num;
        PTE *vpage;
        int pid;
        int exit;
        unsigned int age;
        int tau;

        Frame(int i){
            frame_num = i;
            vpage = nullptr;
            pid = -1;
            exit = 0;
            age = 0;
            tau = 0;
        }
};

//Structure to hold instructions
class Instruction {
    public:
        char ins;
        int val;
};

//Useful data to be used globally
vector<Process*> process_list;
vector<Frame*> frame_table;
queue<Frame*> free_list;
vector<Instruction*> instructions;
Process *current_process = nullptr;
VMA *current_VMA = nullptr;

//Base class Pager
class Pager{
    public:
        virtual Frame* select_victim_frame() = 0;
};

//FIFO algorithm based implementation of Pager
class FIFO : public Pager {
    public:
        int frame_num;
        FIFO(){
            frame_num = 0;
        }
        
        Frame* select_victim_frame(){
            frame_num = frame_num % num_of_frames;
            Frame *f = frame_table[frame_num];
            frame_num = (frame_num + 1) % num_of_frames;
            return f;
        }
};

//Random algorithm based implementation of Pager
class Random : public Pager {
    public:
        int frame_num;
        Random(){
            frame_num = 0;
        }

        Frame* select_victim_frame(){
            frame_num = myrandom(num_of_frames);
            Frame *f = frame_table[frame_num-1];
            return f;
        }
};

//Clock algorithm based implementation of Pager
class Clock : public Pager {
    public:
        int frame_num;
        Clock(){
            frame_num = 0;
        }

        Frame* select_victim_frame(){
            Frame *f;
            bool flag = false;
            for(int i=0;i<num_of_frames*2;i++){
                if(frame_table[frame_num]->vpage->reference){
                    frame_table[frame_num]->vpage->reference = 0;
                }
                else{
                    f=frame_table[frame_num];
                    flag = true;
                }
                frame_num = (frame_num + 1) % num_of_frames;
                if(flag){
                    return f;
                }
            }
            return nullptr;
        }
};

//Enhance NRU algorithm based implementation of Pager
class NRU : public Pager {
    public:
        int frame_num;
        int last_NRU;
        NRU(){
            frame_num = 0;
            last_NRU = 0;
        }

        Frame* select_victim_frame(){
            Frame *f = nullptr;
            bool reset = false;
            vector<Frame*> classes;
            if((instruction_counter) - last_NRU >= 50){
                reset = true;
                last_NRU = instruction_counter;
            }

            for(int i=0;i<4;i++){
                classes.push_back(nullptr);
            }
            
            //Fill up classes with max one frame in each index
            //End loop if frame with R=0 & M=0 found.
            //if More than 50 instructions have completed after last victim,
            //reference bit is reset for all frames.
            for(int i=0;i<num_of_frames;i++){
                int class_index = ((frame_table[frame_num]->vpage->reference)*2) + 
                                    ((frame_table[frame_num]->vpage->modified));
                if(!classes[class_index]){
                    classes[class_index] = frame_table[frame_num];
                }
                if(reset){
                    frame_table[frame_num]->vpage->reference = 0;
                }
                else if(classes[0]){
                    break;
                }
                frame_num = (frame_num + 1) % num_of_frames;
            }
            
            //Find the first frame in by searching in class 0, then 1,2 and 3.
            for(int i=0;i<4;i++){
                if((f = classes[i])){
                    break;
                }
            }
            frame_num = (f->frame_num + 1) % num_of_frames;
            return f;

        }
};

//Aging algorithm based implementation of Pager
class Aging : public Pager {
    public:
        int frame_num;
        Aging(){
            frame_num = 0;
        }
        Frame* select_victim_frame(){
            Frame *f = frame_table[frame_num];
            for(int i=0;i<num_of_frames;i++){
                //Right shift age
                frame_table[frame_num]->age = frame_table[frame_num]->age >> 1;
                if(frame_table[frame_num]->vpage->reference){
                    //Reset age and reference bit
                    frame_table[frame_num]->age = (frame_table[frame_num]->age | 0x80000000);
                    frame_table[frame_num]->vpage->reference=0;
                }
                //Choose the frame with least age
                if(frame_table[frame_num]->age < f->age){
                    f = frame_table[frame_num];
                }
                frame_num = (frame_num + 1) % num_of_frames;
            }
            frame_num = (f->frame_num + 1) % num_of_frames;
            return f;
        }
};

//Working Set algorithm based implementation of Pager
class WS : public Pager {
    public:
        int frame_num;
        int last_NRU;
        WS(){
            frame_num = 0;
            last_NRU = 0;
        }

        Frame* select_victim_frame(){
            Frame *f = nullptr;
            int initial_frame_num = frame_num;
            int max_diff_so_far = 0;
            for(int i=0;i<num_of_frames;i++){
                Frame *curr = frame_table[frame_num];
                int diff = instruction_counter - curr->tau;
                if(curr->vpage->reference){
                    //Update tau to the current instruction count
                    curr->tau = instruction_counter;
                    curr->vpage->reference = 0;
                }
                //If frame was used in an instruction more than 49 instructions before
                //That frame is chosen
                else if(diff>=50){
                    f = curr;
                    break;
                }
                //Otherwise we choose the frame that was least recently used
                else if(diff>max_diff_so_far){
                    max_diff_so_far = diff;
                    f = curr;
                }
                frame_num = (frame_num + 1) % num_of_frames;
            }
            //If a frame was not found based on working set, 
            //the initial frame when starting the search is chosen
            if(!f){
                f = frame_table[initial_frame_num];
            }
            frame_num = (f->frame_num + 1) % num_of_frames;
            return f;

        }
};

//Pager object to refer the chosen Paging algorithm
Pager *pager;

//Print IN when option_O is chosen and updates stats
void print_in(int pid){
    if(print_O){
        cout<<" IN\n";
    }
    pstats[pid]->in_count++;
}
//Print FIN when option_O is chosen and updates stats
void print_fin(int pid){
    if(print_O){
        cout<<" FIN\n";
    }
    pstats[pid]->fin_count++;
}
//Print OUT when option_O is chosen and updates stats
void print_out(int pid){
    if(print_O){
        cout<<" OUT\n";
    }
    pstats[pid]->out_count++;
}
//Print FOUT when option_O is chosen and updates stats
void print_fout(int pid){
    if(print_O){
        cout<<" FOUT\n";
    }
    pstats[pid]->fout_count++;
}
//Print ZERO when option_O is chosen and updates stats
void print_zero(int pid){
    if(print_O){
        cout<<" ZERO\n";
    }
    pstats[pid]->zero_count++;
}
//Print MAP when option_O is chosen and updates stats
void print_map(int pid, int frame){
    if(print_O){
        cout<<" MAP "<<frame<<endl;
    }
    pstats[pid]->map_count++;
}
//Print UNMAP when option_O is chosen and updates stats
void print_unmap(int pid, int vpage){
    if(print_O){
        cout<<" UNMAP "<<pid<<":"<<vpage<<endl;
    }
    pstats[pid]->unmap_count++;
}
//Print SEGV when option_O is chosen and updates stats
void print_segv(int pid){
    if(print_O){
        cout<<" SEGV\n";
    }
    pstats[pid]->segv_count++;
}
//Print SEGPROT when option_O is chosen and updates stats
void print_segprot(int pid){
    if(print_O){
        cout<<" SEGPROT\n";
    }
    pstats[pid]->segprot_count++;
}
//Print instruction details when carrying out operation if option_O is chosen
void print_instruction(int instruction_num, char operation, int vpage){
    if(print_O){
        cout<<instruction_num<<": ==> "<<operation<<" "<<vpage<<endl;
    }
}
//Print EXIT when option_O is chosen and updates stats
void print_exit(int pid){
    if(print_O){
        cout<<"EXIT current process "<<pid<<endl;
    }
    pstats[pid]->exit_count++;
}

//Allocates frame from free list which operates as a queue
Frame* allocate_frame_from_free_list(){
    if(free_list.empty()){
        return nullptr;
    }
    else{
        Frame *f = free_list.front();
        free_list.pop();
        return f;
    }
}

//Returns a frame from free list. If no frames available in free list,
//paging algorithm chooses a victim frame to swap in physical memory
Frame* get_frame(){
    Frame *f = allocate_frame_from_free_list();
    if(f==nullptr){
        f = pager->select_victim_frame();
    }
    return f;
}

//Returns next instruction and increments counter
Instruction* get_next_instruction(){
    if(instruction_counter>=instructions.size()){
        return nullptr;
    }
    return instructions[instruction_counter++];
}

//Returns VMA for current vpage from instruction on current process
VMA* get_VMA(){
    for(int i=0;i<current_process->VMA_list.size();i++){
        if(current_process->VMA_list[i]->starting_vpage<=vpage && current_process->VMA_list[i]->ending_vpage>=vpage){
            return current_process->VMA_list[i];
        }
    }
    return nullptr;
}

//Maps Frame and PTE and performs IN, FIN or ZERO operation based on PTE bits information
//The PTE becomes valid only when it is assigned to a frame and its values are updated here
//PTE was only initialized before, but never updated values based on its corresponding VMA until it is mapped.
void map(Frame *assign_frame){
    assign_frame->pid = current_process->pid;
    assign_frame->age = 0;
    assign_frame->tau = instruction_counter-1;
    PTE *pte = current_process->page_table[vpage];
    pte->valid = 1;
    pte->write_protect = (unsigned int)current_VMA->write_protected;
    pte->file_mapped = (unsigned int)current_VMA->file_mapped;
    pte->virutal_address = (unsigned int)vpage;
    pte->num_of_physical_frames = (unsigned int)assign_frame->frame_num;
    pte->reference = 0;
    pte->modified = 0;
    assign_frame->vpage = pte;
    if(pte->file_mapped){
        print_fin(current_process->pid);
    }
    else if(pte->paged_out){
        print_in(current_process->pid);
    }
    else{
        print_zero(current_process->pid);
    }
    print_map(current_process->pid,assign_frame->frame_num);
}

//Unmap if the frame was previously assigned with vpage
//Based on the information in PTE bits of vpage pointed by frame, 
//OUT, FOUT operations are performed before swapping it out.
void unmap(Frame *unassign_frame){
    if(unassign_frame->vpage){
        PTE *pte = unassign_frame->vpage;
        print_unmap(unassign_frame->pid,pte->virutal_address);
        pte->valid=0;
        if(pte->modified){
            if(pte->file_mapped){
                print_fout(unassign_frame->pid);
            }
            else{
                print_out(unassign_frame->pid);
                pte->paged_out=1;
            }
        }
    }
}

//Updates PTE bits by performing the operation asked in instruction
//Read, write operations are done, but if PTE is write protected, SEGPROT message is printed
//and PTE is still considered unmodified although reference bit is set to 1.
void update_pte(PTE *pte){
    switch(operation){
    case 'r':
        pte->read(current_process->pid);
        break;
    case 'w':
        if(pte->write_protect){
            print_segprot(current_process->pid);
            pstats[current_process->pid]->write_count++;
        }
        else{
            pte->write(current_process->pid);
        }
        pte->reference = 1;
        break;
    default:
        break;
    }
}

//When a process exits, all pages used by the process are freed by umapping them
//if corresponding PTE was modified and filemapped, FOUT is performed before 
//the PTE is paged out.
void exits(){
    for(int i=0;i<current_process->page_table.size();i++){
        PTE *pte = current_process->page_table[i];
        if(pte->valid){
            pte->valid = 0;
            print_unmap(current_process->pid,pte->virutal_address);
            free_list.push(frame_table[pte->num_of_physical_frames]);
            frame_table[pte->num_of_physical_frames]->vpage = nullptr;
            if(pte->modified && pte->file_mapped){
                print_fout(current_process->pid);
            }
        }
        pte->paged_out = 0;
    }
}

//Simulates paging - Takes in instructions one by one
//Performs the operation on the vpage (from instruction) of the current process
//Gets the VMA for the vpage/PTE, if vpage is in hole (has no suitable VMA) SEGV is printed
//Although, read write operations could not be done with vpage in hole, their count was incremented for stats
//If VMA is found, we get a frame for the vpage. If it previously was mapped, 
//it gets unmapped and then gets mapped to vpage (from instruction).
//If the PTE is already assigned (valid) for the vpage, PTE is updated based on the operation.
void simulation(){
    Instruction *inst = nullptr;
    while((inst = get_next_instruction())){
        operation = inst->ins;
        vpage = inst->val;
        print_instruction(instruction_counter-1,operation,vpage);
        if(operation=='c'){
            current_process = process_list[vpage];
            pstats[current_process->pid]->context_switch_count++;
            continue;
        }
        else if(operation=='e'){
            print_exit(current_process->pid);
            exits();
            continue;
        }
        PTE *pte = current_process->page_table[vpage];
        
        if(!pte->valid){
            if(!(current_VMA = get_VMA())){
                print_segv(current_process->pid);
                switch(operation){
                case 'r':
                    pstats[current_process->pid]->read_count++;
                    break;
                case 'w':
                    pstats[current_process->pid]->write_count++;
                    break;
                default:
                    break;
                }
                continue;
            }
            else{
                Frame *f = get_frame();
                unmap(f);
                map(f);
            }
        }
        update_pte(pte);
    }
}

//using input to update variables used in program
void decrypt_input(){
    int i=0;
    num_of_frames=0;
    while(isdigit(input_num_of_frames[i])){
        num_of_frames = num_of_frames*10 + (input_num_of_frames[i] - 48);
        i++;
    }
    algo = input_algo[0];
    i=0;
    while(isalpha(input_OPFS[i])){
        switch(input_OPFS[i]){
        case 'O':
            print_O = true;
            break;
        case 'P':
            print_P = true;
            break;
        case 'F':
            print_F = true;
            break;
        case 'S':
            print_S = true;
            break;
        case 'x':
            x_flag = true;
            break;
        case 'y':
            y_flag = true;
            break;
        case 'f':
            f_flag = true;
            break;
        case 'a':
            a_flag = true;
            break;
        default:
            break;
        }
        i++;
    }
}

//Reads Processes, VMAs and instructions from input file
void read_input(){
    //Get number of processes
    while(current_index<size_str&&(!isdigit(str[current_index])&&!isalpha(str[current_index]))){current_index++;}
    int z = 0;
    while(current_index<size_str&&isdigit(str[current_index])){
        z = z*10 + (str[current_index] - 48);
        current_index++;
    }
    num_of_processes = z;
    //Reading Processes
    for(int i=0;i<num_of_processes;i++){
        while(current_index<size_str&&(!isdigit(str[current_index])&&!isalpha(str[current_index]))){current_index++;}
        int y = 0;
        while(current_index<size_str&&isdigit(str[current_index])){
            y = y*10 + (str[current_index] - 48);
            current_index++;
        }
        int num_of_VMA = y;
        Process *proc = new Process(i);
        //Reading VMAs for process
        for(int j=0;j<num_of_VMA;j++){
            VMA *temp = new VMA();
            while(current_index<size_str&&(!isdigit(str[current_index])&&!isalpha(str[current_index]))){current_index++;}
            int x = 0;
            while(current_index<size_str&&isdigit(str[current_index])){
                x = x*10 + (str[current_index] - 48);
                current_index++;
            }
            temp->starting_vpage = x;
            x=0;
            while(current_index<size_str&&(!isdigit(str[current_index])&&!isalpha(str[current_index]))){current_index++;}
            while(current_index<size_str&&isdigit(str[current_index])){
                x = x*10 + (str[current_index] - 48);
                current_index++;
            }
            temp->ending_vpage = x;
            x=0;
            while(current_index<size_str&&(!isdigit(str[current_index])&&!isalpha(str[current_index]))){current_index++;}
            while(current_index<size_str&&isdigit(str[current_index])){
                x = x*10 + (str[current_index] - 48);
                current_index++;
            }
            temp->write_protected = x;
            x=0;
            while(current_index<size_str&&(!isdigit(str[current_index])&&!isalpha(str[current_index]))){current_index++;}
            while(current_index<size_str&&isdigit(str[current_index])){
                x = x*10 + (str[current_index] - 48);
                current_index++;
            }
            temp->file_mapped = x;
            x=0;
            proc->VMA_list.push_back(temp);
        }
        //Initializing Page table for process
        for(int j=0;j<MAX_VPAGE_SIZE;j++){
            PTE *temp = new PTE();
            proc->page_table.push_back(temp);
        }
        
        process_list.push_back(proc);
        Pstats *init_s = new Pstats();
        pstats.push_back(init_s);
    }

    //Read instructions
    while(current_index<size_str){
        while(current_index<size_str&&(!isdigit(str[current_index])&&!isalpha(str[current_index]))){current_index++;}
        Instruction *instruct = new Instruction();
        if(isalpha(str[current_index])){
            instruct->ins = str[current_index];
            current_index++;
        
            while(current_index<size_str&&(!isdigit(str[current_index])&&!isalpha(str[current_index]))){current_index++;}
            int x = 0;
            while(current_index<size_str&&isdigit(str[current_index])){
                x = x*10 + (str[current_index] - 48);
                current_index++;
            }
            instruct->val = x;
            instructions.push_back(instruct);
        }
    }
}

//Prints input to test if the input was taken correctly
//Only for testing purpose - Not required in grading
void print_input(){
    cout<<"Number of processes = "<<num_of_processes<<endl;
    for(int i=0;i<num_of_processes;i++){
        int n = process_list[i]->VMA_list.size();
        cout<<"Num of VMA = "<<n<<endl;
        for(int j=0;j<n;j++){
            cout<<"sp: "<<process_list[i]->VMA_list[j]->starting_vpage<<" ep: "<<process_list[i]->VMA_list[j]->ending_vpage
                    <<" wp: "<<process_list[i]->VMA_list[j]->write_protected<<" fm: "<<process_list[i]->VMA_list[j]->file_mapped<<endl;
        }
    }
    cout<<"Instructions:"<<endl;
    for(int i=0;i<instructions.size();i++){
        cout<<instructions[i]->ins<<" "<<instructions[i]->val<<endl;
    }
}

//Initialize the physical frame table based in chosen number of frames
//Also, insert all these frames in free list as initially all frames are free
void initialize(){
    for(int i=0;i<num_of_frames;i++){
        Frame *f = new Frame(i);
        frame_table.push_back(f);
    }
    
    for(int i=0;i<frame_table.size();i++){
        Frame *f = frame_table[i];
        free_list.push(f);   
    }
}

//Print Summary if option_S is chosen
void print_for_option_S(){
    long long unsigned int context_switch_ct = 0;
    long long unsigned int exit_ct = 0;
    long long unsigned int map_unmap_ct = 0;
    long long unsigned int in_out_ct = 0;
    long long unsigned int fin_fout_ct = 0;
    long long unsigned int zero_ct = 0;
    long long unsigned int segv_ct = 0;
    long long unsigned int segprot_ct = 0;
    long long unsigned int read_write_ct = 0;

    for(int i=0;i<process_list.size();i++){
        int pid = process_list[i]->pid;
        cout<<"PROC["<<process_list[i]->pid<<"]: U="<<pstats[pid]->unmap_count<<" M="<<
            pstats[pid]->map_count<<" I="<<pstats[pid]->in_count<<" O="<<pstats[pid]->out_count<<
            " FI="<<pstats[pid]->fin_count<<" FO="<<pstats[pid]->fout_count<<" Z="<<
            pstats[pid]->zero_count<<" SV="<<pstats[pid]->segv_count<<" SP="<<pstats[pid]->segprot_count<<endl;

        context_switch_ct += pstats[pid]->context_switch_count;
        exit_ct += pstats[pid]->exit_count;
        map_unmap_ct += pstats[pid]->map_count + pstats[pid]->unmap_count;
        in_out_ct += pstats[pid]->in_count + pstats[pid]->out_count;
        fin_fout_ct += pstats[pid]->fin_count + pstats[pid]->fout_count;
        zero_ct += pstats[pid]->zero_count;
        segv_ct += pstats[pid]->segv_count;
        segprot_ct += pstats[pid]->segprot_count;
        read_write_ct += pstats[pid]->read_count + pstats[pid]->write_count;
    }
    long long int cost = ((map_unmap_ct)*400) +
                            ((in_out_ct)*3000) +
                            ((fin_fout_ct)*2500) +
                            ((zero_ct)*150) + ((segv_ct)*240) +
                            ((segprot_ct)*300) + ((read_write_ct)*1) +
                            ((context_switch_ct)*121) + ((exit_ct)*175);
    cout<<"TOTALCOST "<<instruction_counter<<" "<<context_switch_ct<<" "<<exit_ct<<" "<<cost<<endl;
}

//Print Frame table if option_F is chosen
void print_for_option_F(){
    cout<<"FT: ";
    for(int i=0;i<frame_table.size();i++){
        if(frame_table[i]->vpage){
            cout<<frame_table[i]->pid<<":"<<frame_table[i]->vpage->virutal_address<<" ";
        }
        else{
            cout<<"* ";
        }
    }
    cout<<endl;
}

//Print Page table if option_P is chosen
void print_for_option_P(){
    for(int i=0;i<process_list.size();i++){
        cout<<"PT["<<process_list[i]->pid<<"]: ";
        for(int j=0;j<process_list[i]->page_table.size();j++){
            PTE *pte = process_list[i]->page_table[j];
            if(pte->valid){
                char ref = '-';
                char mod = '-';
                char swap = '-';
                if(pte->reference){
                    ref = 'R';
                }
                if(pte->modified){
                    mod = 'M';
                }
                if(pte->paged_out){
                    swap = 'S';
                }
                cout<<j<<":"<<ref<<mod<<swap<<" ";
            }
            else if(pte->paged_out){
                cout<<"# ";
            }
            else{
                cout<<"* ";
            }
        }
        cout<<endl;
    }
}

//Handles input, options, calls initializers, simulation and printers
int main(int argc, char* argv[]){

    //Input from file
    ifstream input_file(argv[argc-2]);
    if ( argc<3)
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

    //Input of random numbers from file
    ifstream random_file(argv[argc-1]);
    int next = 0;
    random_file>>next;
    while (random_file >> next) {
        randvals.push_back(next);
    }
    
    int c;
    //getting options from command line inputs
    while ((c = getopt (argc, argv, "a:o:f:")) != -1)
        switch (c){
            case 'f':
                input_num_of_frames = input_num_of_frames + optarg;
                break;
            case 'a':
                input_algo = input_algo + optarg;
                break;
            case 'o':
                input_OPFS = input_OPFS + optarg;
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
    
    //Based on the chosen options, input is decrypted to initialize relevant variables
    decrypt_input();
    //Input from file is read and stored in relavent variables
    read_input();

    //Only for testing
    //print_input();
    
    //Initialize frame table and free list
    initialize();

    //Paging algorithm is chosen and pager object will refer to the chosen algorithm
    switch(algo){
        case 'f':
            pager = new FIFO();
            break;
        case 'r':
            pager = new Random();
            break;
        case 'c':
            pager = new Clock();
            break;
        case 'e':
            pager = new NRU();
            break;
        case 'a':
            pager = new Aging();
            break;
        case 'w':
            pager = new WS();
            break;
        default:
            pager = new FIFO();
            break;
    }
    
    //Perform simulations based on instructions
    simulation();

    //Print based on chosen options
    if(print_P){
        print_for_option_P();
    }
    if(print_F){
        print_for_option_F();
    }
    if(print_S){
        print_for_option_S();
    }

    return 0;
}
