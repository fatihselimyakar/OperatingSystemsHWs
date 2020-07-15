#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<math.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

/* Defines for statistic array */
#define FILL 0
#define QUICK 1
#define BUBBLE 2
#define MERGE 3
#define INDEX 4
#define CHECK 5

/* Functions */
unsigned long get_time_microseconds();
void print_page_table();
void initialize_memories();
void fill();
void fifo_algorithm(int disk_page_num,int quarter);
void second_chance_algorithm(int disk_page_num,int quarter);
void LRU_algorithm(int disk_page_num,int quarter);
void WSClock_algorithm(int disk_page_num,int quarter);
void NRU_algorithm(int disk_page_num,int quarter);
void set(unsigned int index, int value, char * tName);
int get_direct_disk(unsigned int index);
int get_disk(unsigned int index);
int get(unsigned int index, char * tName);
void get_page_in_disk(int disk_page_num,int physical_page_num);
int is_hold_physical_mem(int index);
int get_physical(unsigned int index);
void free_memories();
void bubble_sort(int* virtual_array,int start_index, int size);
void index_sort(int* virtual_array,int start_index, int size);
int partition (int* virtual_array, int left, int right);
void quick_sort(int* virtual_array, int start_index, int end_index);
void merge(int* virtual_array, int start_index, int mid_index, int end_index);
void merge_sort(int* virtual_array, int start_index, int end_index);
int check_sorting(int start_index,int end_index,char* tName);
void print_statistics();
void* thread_func(void* arg);

/* Page table entry struct */
struct page_table_entry{
    int lru_used;
    int holding_page;
    int reference_bit;
    int modified_bit;
    int is_present;
    unsigned long recent_access_time;
};

/* Statistic struct to hold statistic */
struct statistics{
    int reads;
    int writes;
    int page_misses;
    int page_replacement;
    int disk_writes;
    int disk_reads;
};

/* Virtual,Physical memory array and page tables */
struct page_table_entry* physical_page_array;
struct page_table_entry* virtual_page_array;
int* virtual_memory;
int* physical_memory;

/* Indexes for replacement algorithms */
int fifo_index[4];
int second_chance_index[4];
int ws_clock_index[4];

/* Command line input variables */
int frame_size;
int num_physical;
int num_virtual;
char page_replacement[20];
char alloc_policy[20];
int page_table_print_int;
char disk_file_name[500];

/* Page variable */
int physical_mem_page_num;
int virtual_mem_page_num;

/* Integer sizes */
int frame_size_int;
int num_physical_int;
int num_virtual_int;

/* Thread and mutex to create critical region */
pthread_t tid[4];
pthread_mutex_t mutex;
int thread_no[4];

/* Memory access and fill,quick,bubble,merge,index,check statistics */
int memory_access;
struct statistics statistics[6];

/* lru counter */
int lru_counter;

/* Gets time according to microseconds */
unsigned long get_time_microseconds(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return 1000000 * tv.tv_sec + tv.tv_usec;;
}

/* Prints the page table */
void print_page_table(){
    int i;
    printf("***PAGE TABLE PRINT***\n");
    for(i=0;i<virtual_mem_page_num;++i){
        printf("index:%d,holding page:%d,reference bit:%d,modified bit %d,present bit:%d,recent access time %ld\n",i,virtual_page_array[i].holding_page,virtual_page_array[i].reference_bit,virtual_page_array[i].modified_bit,virtual_page_array[i].is_present,virtual_page_array[i].recent_access_time);
    }
    printf("\n");
}

/* Initializes the memories and variables */
void initialize_memories(){
    int i;
    /* Memory variables initializes */
    frame_size_int=pow(2,frame_size);
    physical_mem_page_num=pow(2,num_physical);
    virtual_mem_page_num=pow(2,num_virtual);
    num_physical_int=frame_size_int*physical_mem_page_num;
    num_virtual_int=frame_size_int*virtual_mem_page_num;

    /* Memory initializes */
    virtual_memory=(int*)calloc(num_virtual_int,sizeof(int));
    physical_memory=(int*)calloc(num_physical_int,sizeof(int));
    physical_page_array=(struct page_table_entry*)calloc(physical_mem_page_num,sizeof(struct page_table_entry));
    virtual_page_array=(struct page_table_entry*)calloc(virtual_mem_page_num,sizeof(struct page_table_entry));
    
    /* Statistic variable's initializes */
    for(i=0;i<6;++i){
        statistics[i].disk_reads=0;
        statistics[i].disk_writes=0;
        statistics[i].page_misses=0;
        statistics[i].page_replacement=0;
        statistics[i].reads=0;
        statistics[i].writes=0;
    }

    /* Indexes and memory_access initializes */
    fifo_index[0]=0; fifo_index[1]=0; fifo_index[2]=0;  fifo_index[3]=0; 
    second_chance_index[0]=0; second_chance_index[1]=0; second_chance_index[2]=0; second_chance_index[3]=0;
    ws_clock_index[0]=0; ws_clock_index[1]=0; ws_clock_index[2]=0; ws_clock_index[3]=0;
    memory_access=0;
    lru_counter=0;
}

/* Fills the initialized variables and memory randomly */
void fill(){
    int i,j;
    /* Opens the disk file for writing disk */
    FILE* disk_file=fopen(disk_file_name,"w+");
    if(disk_file==NULL){
        perror("Fopen error.");
        exit(EXIT_FAILURE);
    }
    srand(1000);
    /* Fills the virtual memory addresses */
    for(i=0;i<(num_virtual_int/4)*3;++i){
        virtual_memory[i]=i;
        ++statistics[FILL].writes;
    }
    for(j=num_virtual_int-1;i<num_virtual_int;++i,--j){
        virtual_memory[i]=j;
        ++statistics[FILL].writes;
    }

    /* Fills virtual memory values(in disk and physical memory) randomly */
    for(i=0;i<num_physical_int;++i){
        physical_memory[i]=rand();
        fprintf(disk_file,"%.10d\n",physical_memory[i]);
        ++statistics[FILL].writes;
    }
    for(;i<num_virtual_int;++i){
        fprintf(disk_file,"%.10d\n",rand());
        ++statistics[FILL].disk_writes;
    }

    /* Fills page tables */
    for(i=0;i<physical_mem_page_num;++i){
        physical_page_array[i].lru_used=0;
        physical_page_array[i].holding_page=i;
        physical_page_array[i].modified_bit=0;
        physical_page_array[i].recent_access_time=get_time_microseconds();
        physical_page_array[i].reference_bit=0;
    }

    for(i=0;i<virtual_mem_page_num;++i){
        virtual_page_array[i].lru_used=0;
        virtual_page_array[i].holding_page=i;
        virtual_page_array[i].modified_bit=0;
        virtual_page_array[i].recent_access_time=get_time_microseconds();
        virtual_page_array[i].reference_bit=0;
    }

    /* Closes disk file */
    if(fclose(disk_file)!=0){
        perror("Fclose error.");
        exit(EXIT_FAILURE);
    }
}

/* Applies fifo page replacement algorithm based on fifo index. If it is local, it replaces only 1/4 of the physical memory, if it is global, it replaces all of it. */
void fifo_algorithm(int disk_page_num,int quarter){
    /* Local policy */
    if(strcmp(alloc_policy,"local")==0){
        int quarter_of_physicals=(physical_mem_page_num/4);
        int start_index=quarter*quarter_of_physicals;
        if(fifo_index[quarter]==0){
            fifo_index[quarter]=start_index;
        }
        get_page_in_disk(disk_page_num,fifo_index[quarter]);
        ++fifo_index[quarter];
        fifo_index[quarter]%=(start_index+quarter_of_physicals);
    }
    /* Global policy */
    else if(strcmp(alloc_policy,"global")==0){
        get_page_in_disk(disk_page_num,fifo_index[0]);
        ++fifo_index[0];
        fifo_index[0]%=physical_mem_page_num;
    }
}

/* It applies the second chance page replacement algorithm based on the sc index. In addition to the phyto, instead of directly erasing it, it gives a second chance by pulling the reference bit from 1 to 0. If it is local, it replaces only 1/4 of the physical memory, if it is global, it replaces all of it. */
void second_chance_algorithm(int disk_page_num,int quarter){
    /* Local policy */
    if(strcmp(alloc_policy,"local")==0){
        int quarter_of_physicals=(physical_mem_page_num/4);
        int start_index=quarter*quarter_of_physicals;
        if(second_chance_index[quarter]==0){
            second_chance_index[quarter]=start_index;
        }
        while(physical_page_array[second_chance_index[0]].reference_bit!=0){
            physical_page_array[second_chance_index[0]].reference_bit=0;
            ++second_chance_index[0];
            second_chance_index[0]%=physical_mem_page_num;
            if(second_chance_index[quarter]==0){
                second_chance_index[quarter]=start_index;
            }
        }
        get_page_in_disk(disk_page_num,second_chance_index[quarter]);
        ++second_chance_index[quarter];
        second_chance_index[quarter]%=(start_index+quarter_of_physicals);
    }
    /* Global policy */
    else if(strcmp(alloc_policy,"global")==0){
        while(physical_page_array[second_chance_index[0]].reference_bit!=0){
            physical_page_array[second_chance_index[0]].reference_bit=0;
            ++second_chance_index[0];
            second_chance_index[0]%=physical_mem_page_num;
        }
        get_page_in_disk(disk_page_num,second_chance_index[0]);
        ++second_chance_index[0];
        second_chance_index[0]%=physical_mem_page_num;
    }
}

/* Resets all page numbers used for a specified amount of time */
void flush_lru_used(){
    int i;
    for(i=0;i<physical_mem_page_num;++i){
        physical_page_array[i].lru_used=0;
    }
}

/* Applies last recently used algorithm. Finds the oldest and replaces it by looking at the recent access time in the page tables in the physical memory. It uses 1/4 of Local memory and uses all if it is global. */
void LRU_algorithm(int disk_page_num,int quarter){
    int i;
    int min=999999999;
    int min_index=0;
    /* Local policy */
    if(strcmp(alloc_policy,"local")==0){
        int quarter_of_physicals=(physical_mem_page_num/4);
        int start_index=quarter*quarter_of_physicals;
        for(i=0;i<quarter_of_physicals;++i){
            if(physical_page_array[i+start_index].lru_used<min){
                min=physical_page_array[i+start_index].lru_used;
                min_index=i+start_index;
            }
        }
        ++lru_counter;
        if(lru_counter%1000==0)
            flush_lru_used();
        get_page_in_disk(disk_page_num,min_index);
    }
    /* Global policy */
    else if(strcmp(alloc_policy,"global")==0){
        for(i=0;i<physical_mem_page_num;++i){
            if(physical_page_array[i].lru_used<min){
                min=physical_page_array[i].lru_used;
                min_index=i;
            }
        }
        ++lru_counter;
        if(lru_counter%1000==0)
            flush_lru_used();
        get_page_in_disk(disk_page_num,min_index);
    }
}

/* Applies the WSClock algorithm. It circulates circularly according to the wsclock index and pulls 0 to 0 if reference is 1, and replaces that page. If it is local, it uses 1/4 of physical memory. If it is global, it uses the whole memory. */
void WSClock_algorithm(int disk_page_num,int quarter){
    /* Local policy */
    if(strcmp(alloc_policy,"local")==0){
        int quarter_of_physicals=(physical_mem_page_num/4);
        int start_index=quarter*quarter_of_physicals;
        if(ws_clock_index[quarter]==0){
            ws_clock_index[quarter]=start_index;
        }
        while(physical_page_array[ws_clock_index[0]].reference_bit!=0){
            physical_page_array[ws_clock_index[0]].reference_bit=0;
            ++ws_clock_index[0];
            ws_clock_index[0]%=physical_mem_page_num;
            if(ws_clock_index[quarter]==0){
                ws_clock_index[quarter]=start_index;
            }
        }
        get_page_in_disk(disk_page_num,ws_clock_index[quarter]);
        ++ws_clock_index[quarter];
        ws_clock_index[quarter]%=(start_index+quarter_of_physicals);
    }
    /* Global policy */
    else if(strcmp(alloc_policy,"global")==0){
        while(physical_page_array[ws_clock_index[0]].reference_bit!=0){
            physical_page_array[ws_clock_index[0]].reference_bit=0;
            ++ws_clock_index[0];
            ws_clock_index[0]%=physical_mem_page_num;
        }
        get_page_in_disk(disk_page_num,ws_clock_index[0]);
        ++ws_clock_index[0];
        ws_clock_index[0]%=physical_mem_page_num;
    }
}

/* Applies the Not recently used algorithm. Evaluates pages as 4 classes and replaces the lowest class. */
void NRU_algorithm(int disk_page_num,int quarter){
    int i;
    /* local policy */
    if(strcmp(alloc_policy,"local")==0){
        int quarter_of_physicals=(physical_mem_page_num/4);
        int start_index=quarter*quarter_of_physicals;
        for(i=0;i<quarter_of_physicals;++i){
            if(physical_page_array[i+start_index].reference_bit==0 && physical_page_array[i+start_index].modified_bit==0){
                get_page_in_disk(disk_page_num,i+start_index);
                return;
            }
        }
        for(i=0;i<quarter_of_physicals;++i){
            if(physical_page_array[i+start_index].reference_bit==0 && physical_page_array[i+start_index].modified_bit==1){
                get_page_in_disk(disk_page_num,i+start_index);
                return;
            }
        }
        for(i=0;i<quarter_of_physicals;++i){
            if(physical_page_array[i+start_index].reference_bit==1 && physical_page_array[i+start_index].modified_bit==0){
                get_page_in_disk(disk_page_num,i+start_index);
                return;
            }
        }
        for(i=0;i<quarter_of_physicals;++i){
            if(physical_page_array[i+start_index].reference_bit==1 && physical_page_array[i+start_index].modified_bit==1){
                get_page_in_disk(disk_page_num,i+start_index);
                return;
            }
        }
    }
    /* global policy */
    else if(strcmp(alloc_policy,"global")==0){
        for(i=0;i<physical_mem_page_num;++i){
            if(physical_page_array[i].reference_bit==0 && physical_page_array[i].modified_bit==0){
                get_page_in_disk(disk_page_num,i);
                return;
            }
        }
        for(i=0;i<physical_mem_page_num;++i){
            if(physical_page_array[i].reference_bit==1 && physical_page_array[i].modified_bit==0){
                get_page_in_disk(disk_page_num,i);
                return;
            }
        }
        for(i=0;i<physical_mem_page_num;++i){
            if(physical_page_array[i].reference_bit==0 && physical_page_array[i].modified_bit==1){
                get_page_in_disk(disk_page_num,i);
                return;
            }
        }
        for(i=0;i<physical_mem_page_num;++i){
            if(physical_page_array[i].reference_bit==1 && physical_page_array[i].modified_bit==1){
                get_page_in_disk(disk_page_num,i);
                return;
            }
        }
        
    }
}

/* If called with fill tName, it fills the memory and replaces the line given as index on other options. */
void set(unsigned int index, int value, char * tName){
    ++memory_access;
    /* fills the memory */
    if(strcmp(tName,"fill")==0){
        fill();
    }
    /* Changes the disks indexth line with value */
    else{
        int line_number=virtual_memory[index],page_index;
        FILE *disk=fopen(disk_file_name,"r+");
        pthread_mutex_lock(&mutex);
        if(line_number<0 || line_number>=num_virtual_int){
            perror("set | index wrong");
            exit(EXIT_FAILURE);
        }
        if((page_index=is_hold_physical_mem(index))!=-1){
            int idx=page_index*frame_size_int;
            int offset=virtual_memory[index]%frame_size_int;
            physical_memory[idx+offset]=value;
            physical_page_array[page_index].recent_access_time=get_time_microseconds();
            physical_page_array[page_index].modified_bit=1;
            virtual_page_array[physical_page_array[page_index].holding_page].recent_access_time=get_time_microseconds();
            virtual_page_array[physical_page_array[page_index].holding_page].modified_bit=1;
        }
        fseek(disk,line_number*11,SEEK_SET);
        fprintf(disk,"%.10d\n",value);
        pthread_mutex_unlock(&mutex);
        fclose(disk);
    }
    /* prints if the memory address equal to page table print int */
    if(memory_access%page_table_print_int==0){
        print_page_table();
    }
}

/* It reads and returns the line given as an index directly from the disk. */
int get_direct_disk(unsigned int index){
    char line[100];
    FILE *disk=fopen(disk_file_name,"r");
    if((int)index<0 || (int)index>=num_virtual_int){
        perror("get_direct_disk | index wrong");
        exit(EXIT_FAILURE);
    }
    fseek(disk,index*11,SEEK_SET);
    fgets(line,sizeof(line),disk);
    fclose(disk);
    return atoi(line);
}

/* It reads and returns the line given as an virtual_memory[index] directly from the disk. */
int get_disk(unsigned int index){
    int line_number=virtual_memory[index];
    char line[100];
    FILE *disk=fopen(disk_file_name,"r");
    if(line_number<0 || line_number>=num_virtual_int){
        perror("get_disk | index wrong");
        exit(EXIT_FAILURE);
    }
    fseek(disk,line_number*11,SEEK_SET);
    fgets(line,sizeof(line),disk);
    fclose(disk);
    return atoi(line);
}

/* Finds and returns the address of the given virtual memory index. If the address's values are in physical memory, it returns directly. Otherwise, it copies the page from the disk with the page replacement algorithm and takes the value from there. */
int get(unsigned int index, char * tName){
    int ret_val;
    pthread_mutex_lock(&mutex);
    /* if tName is index directly gets the virtual_memory address */
    if(strcmp(tName,"index")==0){
        ret_val=virtual_memory[index];
       ++statistics[INDEX].reads;
    }
    /* if tName is check directly gets the value without page replacement */
    else if(strcmp(tName,"check")==0 ){
        if((ret_val=get_physical(index))==-1){
            ret_val=get_disk(index);
            ++statistics[CHECK].disk_reads;
            ++statistics[CHECK].page_misses;
        }
        else{
            ++statistics[CHECK].reads;
        }
    }
    /* if tName is fill directly gets the value without page replacement */
    else if(strcmp(tName,"fill")==0){
        if((ret_val=get_physical(index))==-1){
            ret_val=get_disk(index);
            ++statistics[FILL].disk_reads;
            ++statistics[FILL].disk_writes;
            ++statistics[FILL].page_misses;
        }
        else{
            ++statistics[FILL].reads;
        }
    }
    /* Not hitted */
    else if((ret_val=get_physical(index))==-1){
        /* Page replacement part */
        int disk_page=(int)virtual_memory[index]/(int)frame_size_int;
        if(strcmp(tName,"bubble")==0){
            ++statistics[BUBBLE].page_replacement;
            ++statistics[BUBBLE].page_misses;
            ++statistics[BUBBLE].disk_reads;
            ++statistics[BUBBLE].disk_writes;
            statistics[BUBBLE].reads+=2;
        }
        else if(strcmp(tName,"quick")==0){
            ++statistics[QUICK].page_replacement;
            ++statistics[QUICK].page_misses;
            ++statistics[QUICK].disk_reads;
            ++statistics[QUICK].disk_writes;
            statistics[QUICK].reads+=2;
        }
        else if(strcmp(tName,"merge")==0){
            ++statistics[MERGE].page_replacement;
            ++statistics[MERGE].page_misses;
            ++statistics[MERGE].disk_reads;
            ++statistics[MERGE].disk_writes;
            statistics[MERGE].reads+=2;
        }
        else if(strcmp(tName,"index")==0){
            ++statistics[INDEX].page_replacement;
            ++statistics[INDEX].page_misses;
            ++statistics[INDEX].disk_reads;
            ++statistics[INDEX].disk_writes;
            statistics[INDEX].reads+=2;
        }
        if(strcmp(page_replacement,"NRU")==0){
            if(strcmp(tName,"bubble")==0){
                NRU_algorithm(disk_page,0);
            }
            else if(strcmp(tName,"quick")==0){
                NRU_algorithm(disk_page,1);
            }
            else if(strcmp(tName,"merge")==0){
                NRU_algorithm(disk_page,2);
            }
            else if(strcmp(tName,"index")==0){
                NRU_algorithm(disk_page,3);
            }
        }
        else if(strcmp(page_replacement,"FIFO")==0){
            if(strcmp(tName,"bubble")==0){
                fifo_algorithm(disk_page,0);
            }
            else if(strcmp(tName,"quick")==0){
                fifo_algorithm(disk_page,1);
            }
            else if(strcmp(tName,"merge")==0){
                fifo_algorithm(disk_page,2);
            }
            else if(strcmp(tName,"index")==0){
                fifo_algorithm(disk_page,3);
            }
        }
        else if(strcmp(page_replacement,"SC")==0){
            if(strcmp(tName,"bubble")==0){
                second_chance_algorithm(disk_page,0);
            }
            else if(strcmp(tName,"quick")==0){
                second_chance_algorithm(disk_page,1);
            }
            else if(strcmp(tName,"merge")==0){
                second_chance_algorithm(disk_page,2);
            }
            else if(strcmp(tName,"index")==0){
                second_chance_algorithm(disk_page,3);
            }
        }
        else if(strcmp(page_replacement,"LRU")==0){
            if(strcmp(tName,"bubble")==0){
                LRU_algorithm(disk_page,0);
            }
            else if(strcmp(tName,"quick")==0){
                LRU_algorithm(disk_page,1);
            }
            else if(strcmp(tName,"merge")==0){
                LRU_algorithm(disk_page,2);
            }
            else if(strcmp(tName,"index")==0){
                LRU_algorithm(disk_page,3);
            }
        }
        else if(strcmp(page_replacement,"WSClock")==0){
            if(strcmp(tName,"bubble")==0){
                WSClock_algorithm(disk_page,0);
            }
            else if(strcmp(tName,"quick")==0){
                WSClock_algorithm(disk_page,1);
            }
            else if(strcmp(tName,"merge")==0){
                WSClock_algorithm(disk_page,2);
            }
            else if(strcmp(tName,"index")==0){
                WSClock_algorithm(disk_page,3);
            }
        }
        ret_val=get_physical(index);
    }
    /* Hitted */
    else{
        int page_index=is_hold_physical_mem(index);
        if(strcmp(tName,"bubble")==0){
            ++statistics[BUBBLE].reads;
        }
        else if(strcmp(tName,"quick")==0){
            ++statistics[QUICK].reads;
        }
        else if(strcmp(tName,"merge")==0){
            ++statistics[MERGE].reads;
        }
        else if(strcmp(tName,"index")==0){
            ++statistics[INDEX].reads;
        }
        physical_page_array[page_index].recent_access_time=get_time_microseconds();
        physical_page_array[page_index].modified_bit=1;

        virtual_page_array[physical_page_array[page_index].holding_page].recent_access_time=get_time_microseconds();
        virtual_page_array[physical_page_array[page_index].holding_page].modified_bit=1;
    }
    /* if memory_access equals page table print int then prints page table */
    if(memory_access%page_table_print_int==0){
        print_page_table();
    }
    pthread_mutex_unlock(&mutex);
    return ret_val;
}

/* For page replacement, moves the content of a page on disk to a page in the physical memory. */
void get_page_in_disk(int disk_page_num,int physical_page_num){
    int disk_page_index=frame_size_int*disk_page_num;
    int physical_page_index=frame_size_int*physical_page_num;
    int i;

    /* Moving values on disk to physical memory */
    for(i=0;i<frame_size_int;++i){
        physical_memory[i+physical_page_index]=get_direct_disk(i+disk_page_index);
    }

    /* Marks old page's present bit to 0 */
    virtual_page_array[physical_page_array[physical_page_num].holding_page].is_present=0;

    /* Updating the values of page tables */
    physical_page_array[physical_page_num].lru_used=1;
    physical_page_array[physical_page_num].holding_page=disk_page_num;
    physical_page_array[physical_page_num].modified_bit=0;
    physical_page_array[physical_page_num].recent_access_time=get_time_microseconds();
    physical_page_array[physical_page_num].reference_bit=1;
    physical_page_array[physical_page_num].is_present=1;

    virtual_page_array[disk_page_num].lru_used=1;
    virtual_page_array[disk_page_num].holding_page=disk_page_num;
    virtual_page_array[disk_page_num].modified_bit=0;
    virtual_page_array[disk_page_num].recent_access_time=get_time_microseconds();
    virtual_page_array[disk_page_num].reference_bit=1;
    virtual_page_array[disk_page_num].is_present=1;

}

/* Checks whether the index entered is in physical memory. */
int is_hold_physical_mem(int index){
    int page_number=(int)virtual_memory[index]/(int)frame_size_int,i;
    for(i=0;i<physical_mem_page_num;++i){
        if(physical_page_array[i].holding_page==page_number){
            return i;
        }
    }
    return -1;
}

/* If the index parameter exists in physical memory, it returns its value. Returns -1 */
int get_physical(unsigned int index){
    int page_index;
    ++memory_access;
    if((page_index=is_hold_physical_mem(index))==-1 || virtual_memory[index]<0){
        /*perror("get_physical | index wrong");
        exit(EXIT_FAILURE);*/
        return -1;
    }
    else{
        int idx=page_index*frame_size_int;
        int offset=virtual_memory[index]%frame_size_int;
        return physical_memory[idx+offset];
    }   
}

/* frees memory */
void free_memories(){
    free(virtual_memory);
    free(physical_memory);
    free(physical_page_array);
    free(virtual_page_array);
}

/* Sort functions */

/* Bubble sort */
void bubble_sort(int* virtual_array,int start_index, int size){ 
   int i, j; 
   for (i = start_index; i < (start_index+size)-1; i++){
       for (j = start_index; j < (start_index+size)-i-1; j++){
           if (get(j,"bubble") > get(j+1,"bubble")){
               int temp=virtual_array[j];
               virtual_array[j]=virtual_array[j+1];
               virtual_array[j+1]=temp;
               statistics[BUBBLE].writes+=2;
           }      
       } 
   }      
} 

/* Index sort like bubble sort, Unlike other sorts, it sorts virtual memory's indexes (addresses), not its values. */
void index_sort(int* virtual_array,int start_index, int size) 
{ 
   int i, j; 
   for (i = start_index; i < (start_index+size)-1; i++){
       for (j = start_index; j < (start_index+size)-i-1; j++){
           if (get(j,"index") > get(j+1,"index")){
               int temp=virtual_array[j];
               virtual_array[j]=virtual_array[j+1];
               virtual_array[j+1]=temp;
               statistics[INDEX].writes+=2;
           }      
       } 
   }      
} 

/* Quick sort partition function*/
int partition (int* virtual_array, int left, int right){ 
    int middle_pivot = get(right,"quick"); 
    int i = (left - 1),j;
    int temp; 
  
    for (j = left; j <= right- 1; j++){ 
        if (get(j,"quick") < middle_pivot){ 
            i++; 
            temp=virtual_array[i];
            virtual_array[i]=virtual_array[j];
            virtual_array[j]=temp;
            statistics[QUICK].writes+=2;
        } 
    }
    temp=virtual_array[i+1];
    virtual_array[i+1]=virtual_array[right];
    virtual_array[right]=temp;
    statistics[QUICK].writes+=2;
    return (i + 1); 
}

/* Quick sort */
void quick_sort(int* virtual_array, int start_index, int end_index){ 
    if (start_index < end_index){ 
        int middle_pivot = partition(virtual_array, start_index, end_index); 
        quick_sort(virtual_array, start_index, middle_pivot-1); 
        quick_sort(virtual_array, middle_pivot+1, end_index); 
    } 
}

/* Merge sort's merge function */
void merge(int* virtual_array, int start_index, int mid_index, int end_index){ 
    int start2 = mid_index + 1; 
    if (get(mid_index,"merge")<=get(start2,"merge")) { 
        return; 
    } 
    while (start_index<=mid_index && start2<=end_index) { 
        if (get(start_index,"merge") <= get(start2,"merge")) { 
            start_index++; 
        } 
        else { 
            int value = virtual_array[start2]; 
            int index = start2; 

            while (index != start_index) { 
                virtual_array[index] = virtual_array[index-1]; 
                index--; 
                ++statistics[MERGE].writes;
            } 
            virtual_array[start_index] = value; 
            ++statistics[MERGE].writes;

            start_index++; 
            mid_index++; 
            start2++; 
        } 
    } 
}

/* Merge sort function */
void merge_sort(int* virtual_array, int start_index, int end_index){ 
    if (start_index < end_index) { 
        int middle = start_index + (end_index - start_index) / 2; 
        merge_sort(virtual_array, start_index, middle); 
        merge_sort(virtual_array, middle + 1, end_index); 
        merge(virtual_array, start_index, middle, end_index); 
    } 
}

/* Checks the sorted virtual memory quarters. Returns 1 if sorted, 0 if not sorted */
int check_sorting(int start_index,int end_index,char* tName){
    int i;
    if(start_index<0 || start_index>num_virtual_int || end_index<0 || end_index>num_virtual_int){
        perror("index error in check sorting");
        exit(EXIT_FAILURE);
    }
    if(strcmp(tName,"index")==0){
        for(i=start_index;i<end_index-1;++i){
            if(virtual_memory[i]>virtual_memory[i+1]){
                return 0;
            }
        }
    }
    else{
        for(i=start_index;i<end_index-1;++i){
            if(get(i,"check")>get(i+1,"check")){
                return 0;
            }
        }
    }
    
    return 1;
}

/* Prints statistics */
void print_statistics(){
    int i;
    for(i=0;i<6;++i){
        if(i==0){
            printf("\n*FILL STATISTICS*\n");
        }
        else if(i==1){
            printf("\n*QUICK STATISTICS*\n");
        }
        else if(i==2){
            printf("\n*BUBBLE STATISTICS*\n");
        }
        else if(i==3){
            printf("\n*MERGE STATISTICS*\n");
        }
        else if(i==4){
            printf("\n*INDEX STATISTICS*\n");
        }
        else if(i==5){
            printf("\n*CHECK STATISTICS*\n");
        }
        printf("Number of reads:%d\n",statistics[i].reads);
        printf("Number of writes:%d\n",statistics[i].writes);
        printf("Number of page misses:%d\n",statistics[i].page_misses);
        printf("Number of page replacements:%d\n",statistics[i].page_replacement);
        printf("Number of disk writes:%d\n",statistics[i].disk_writes);
        printf("Number of disk reads:%d\n",statistics[i].disk_reads);

    }

}

/* Thread function, Runs the corresponding sorts by quarter. */
void* thread_func(void* arg){
    int quarter=*((int*)arg);
    if(quarter==0){
        bubble_sort(virtual_memory,0,num_virtual_int/4);
    }
    else if(quarter==1){
        quick_sort(virtual_memory,num_virtual_int/4,num_virtual_int/2-1);
    }
    else if(quarter==2){
        merge_sort(virtual_memory,num_virtual_int/2,((num_virtual_int/4)*3)-1);
    }
    else if(quarter==3){
        index_sort(virtual_memory,(num_virtual_int/4)*3,num_virtual_int);
    }
    return NULL;
}

/* Main function */
int main(int argc, char *argv[]){
    int i,j;
    /* Assigning commandline variables.*/
    if(argc!=8){
        errno=EINVAL;
        perror("Number of command line argument must be 8.");
        exit(EXIT_FAILURE);
    }
    else{
        frame_size=atoi(argv[1]);
        num_physical=atoi(argv[2]);
        num_virtual=atoi(argv[3]);
        strcpy(page_replacement,argv[4]);
        strcpy(alloc_policy,argv[5]);
        page_table_print_int=atoi(argv[6]);
        strcpy(disk_file_name,argv[7]);
        if(num_physical>num_virtual){
            errno=EINVAL;
            perror("num_physical must not be greater than num_virtual");
            exit(EXIT_FAILURE);
        }
    }

    /* Printing command line parameters */
    printf("*COMMAND LINE PARAMETERS\n");
    for(i=0;i<argc;++i){
        printf("%s ",argv[i]);
    }
    printf("\n\n");

    /* Initializes and fills the memories and disk */
    initialize_memories();
    set(0,0,"fill");

    /* Initializing the posix mutex */
    if(pthread_mutex_init(&mutex,NULL)!=0){
        perror("Mutex create error");
        exit(EXIT_FAILURE);
    }

    /* Creating four thread for four quarters. */
    for (i = 0; i < 4; ++i) {
        thread_no [i] = i;
        if (pthread_create (&tid[i], NULL, thread_func, (void *) &thread_no [i]) != 0) { 
            perror("Thread create error");
            exit (EXIT_FAILURE);
        }
    }

    /* Joining the threads */
    for (i = 0; i < 4; ++i){
        if (pthread_join (tid[i], NULL) == -1) { 
            perror("Thread join error");
            exit (EXIT_FAILURE);
        }
    }

    /* Destorying the mutexes */
    if(pthread_mutex_destroy(&mutex)!=0){
        perror("Mutex destroy error");
        exit(EXIT_FAILURE);
    }

    /* Printing te sorted values */
    for(i=0,j=0;i<num_virtual_int;++i){
        if(i%(num_virtual_int/4)==0){
            printf("*QUARTER %d*\n",j);
            ++j;
        }
        printf("value:%d index:%d\n",get_disk(i),virtual_memory[i]);
        if((i+1)%(num_virtual_int/4)==0)
            printf("\n");
    }

    /* Printing the checking results */
    printf("*CHECKING THE SORTED PARTS*\n");
    printf("Checking sorted part by bubble:%d\n",check_sorting(0,num_virtual_int/4-1,"bubble"));
    printf("Checking sorted part by quick:%d\n",check_sorting(num_virtual_int/4,num_virtual_int/2-1,"quick"));
    printf("Checking sorted part by merge:%d\n",check_sorting(num_virtual_int/2,3*(num_virtual_int/4)-1,"merge"));
    printf("Checking sorted part by index:%d\n\n",check_sorting(3*(num_virtual_int/4),num_virtual_int,"index"));

    /* Printing the statistics */
    print_statistics();

    /* Freeing the memories */
    free_memories();

    return 0;
}
