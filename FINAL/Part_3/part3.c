#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<math.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

/* Defines for bonus part */
#define NRU 0
#define FIFO 1
#define SC 2
#define LRU 3
#define WSClock 4

unsigned long get_time_microseconds();
void initialize_memory();
void fill_memory();
void reinitialize_and_fill_memory(int fs);
int is_hold_physical_mem(int index);
int get_physical(unsigned int index);
void get_page_in_disk(int disk_page_num,int physical_page_num);
void fifo_algorithm(int disk_page_num);
void second_chance_algorithm(int disk_page_num);
void LRU_algorithm(int disk_page_num);
void WSClock_algorithm(int disk_page_num);
void NRU_algorithm(int disk_page_num);
int get(unsigned int index, char * tName);
void bubble_sort(int* virtual_array,int start_index, int size);
void index_sort(int* virtual_array,int start_index, int size);
int partition (int* virtual_array, int left, int right);
void quick_sort(int* virtual_array, int start_index, int end_index);
void merge(int* virtual_array, int start_index, int mid_index, int end_index);
void merge_sort(int* virtual_array, int start_index, int end_index);
void free_memory();
void find_optimal_page_size();
void find_best_page_replacement();

/* Page table entry struct */
struct page_table_entry{
    int lru_used;
    int holding_page;
    int reference_bit;
    int modified_bit;
    int is_present;
    unsigned long recent_access_time;
};

/* Virtual,Physical memory array,disk and page tables */
int* virtual_memory;
int* physical_memory;
struct page_table_entry* physical_page_array;
int* disk;

/* Input variables */
int page_replacement;
int frame_size;
int virtual_frames;
int virtual_int;
int physical_frames;
int physical_int;

/* Indexes for replacement algorithms */
int fifo_index;
int second_chance_index;
int ws_clock_index;

/* lru counter */
int lru_counter;

/* Nof page faults */
int page_fault;

/* gets time according to microseconds */
unsigned long get_time_microseconds(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return 1000000 * tv.tv_sec + tv.tv_usec;;
}

/* Initializes the memories and variables */
void initialize_memory(){
    virtual_memory=(int*)calloc(virtual_int,sizeof(int));
    disk=(int*)calloc(virtual_int,sizeof(int));
    physical_memory=(int*)calloc(physical_int,sizeof(int));
    physical_page_array=(struct page_table_entry*)calloc(physical_frames,sizeof(struct page_table_entry));
}

/* Fills the initialized variables and memory randomly */
void fill_memory(){
    int i;
    srand(1000);

    /* Fills the virtual memory addresses */
    for(i=0;i<virtual_int;++i){
        virtual_memory[i]=i;
    }

    for(i=0;i<physical_int;++i){
        physical_memory[i]=rand();
        disk[i]=physical_memory[i];
    }

    for(;i<virtual_int;++i){
        disk[i]=rand();
    }

     /* Fills page table */
    for(i=0;i<physical_frames;++i){
        physical_page_array[i].lru_used=0;
        physical_page_array[i].holding_page=i;
        physical_page_array[i].modified_bit=0;
        physical_page_array[i].recent_access_time=get_time_microseconds();
        physical_page_array[i].reference_bit=0;
    }

}

/* Reinitializes and fills the sorts to repeat. */
void reinitialize_and_fill_memory(int fs){
    frame_size=fs;
    virtual_frames=virtual_int/frame_size;
    physical_frames=physical_int/frame_size;
    fifo_index=0;

    printf("reinitialize: frame_size:%d virtual_frames:%d physical frames:%d\n",frame_size,virtual_frames,physical_frames);
    
    /* Frees the old physical page array and creates new one */
    free(physical_page_array);
    physical_page_array=(struct page_table_entry*)calloc(physical_frames,sizeof(struct page_table_entry));

    /* Fills memory again */
    fill_memory();
}

/* Checks whether the index entered is in physical memory. */
int is_hold_physical_mem(int index){
    int page_number=(int)virtual_memory[index]/(int)frame_size,i;
    for(i=0;i<physical_frames;++i){
        if(physical_page_array[i].holding_page==page_number){
            return i;
        }
    }
    return -1;
}

/* If the index parameter exists in physical memory, it returns its value. Returns -1 */
int get_physical(unsigned int index){
    int page_index;
    if((page_index=is_hold_physical_mem(index))==-1 || virtual_memory[index]<0){
        /*perror("get_physical | index wrong");
        exit(EXIT_FAILURE);*/
        return -1;
    }
    else{
        int idx=page_index*frame_size;
        int offset=virtual_memory[index]%frame_size;
        return physical_memory[idx+offset];
    }   
}

/* For page replacement, moves the content of a page on disk to a page in the physical memory. */
void get_page_in_disk(int disk_page_num,int physical_page_num){
    int disk_page_index=frame_size*disk_page_num;
    int physical_page_index=frame_size*physical_page_num;
    int i;

    /* Moving values on disk to physical memory */
    for(i=0;i<frame_size;++i){
        physical_memory[i+physical_page_index]=disk[i+disk_page_index];
    }

    /* Updating the values of page tables */
    physical_page_array[physical_page_num].lru_used=1;
    physical_page_array[physical_page_num].holding_page=disk_page_num;
    physical_page_array[physical_page_num].modified_bit=0;
    physical_page_array[physical_page_num].recent_access_time=get_time_microseconds();
    physical_page_array[physical_page_num].reference_bit=1;
    physical_page_array[physical_page_num].is_present=1;

}

/* Applies global fifo page replacement algorithm based on fifo index. */
void fifo_algorithm(int disk_page_num){
    get_page_in_disk(disk_page_num,fifo_index);
    ++fifo_index;
    fifo_index%=physical_frames;
}

/* It applies the global second chance page replacement algorithm based on the sc index.  */
void second_chance_algorithm(int disk_page_num){
    while(physical_page_array[second_chance_index].reference_bit!=0){
        physical_page_array[second_chance_index].reference_bit=0;
        ++second_chance_index;
        second_chance_index%=physical_frames;
    }
    get_page_in_disk(disk_page_num,second_chance_index);
    ++second_chance_index;
    second_chance_index%=physical_frames;
}

/* Resets all page numbers used for a specified amount of time */
void flush_lru_used(){
    int i;
    for(i=0;i<physical_frames;++i){
        physical_page_array[i].lru_used=0;
    }
}

/* Applies global least recently used algorithm. Finds the oldest and replaces it by looking at the recent access time in the page tables in the physical memory.*/
void LRU_algorithm(int disk_page_num){
    int i;
    int min=999999999;
    int min_index=0;
    for(i=0;i<physical_frames;++i){
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

/* Applies the global WSClock algorithm. It circulates circularly according to the wsclock index and pulls 0 to 0 if reference is 1, and replaces that page.  */
void WSClock_algorithm(int disk_page_num){
    while(physical_page_array[ws_clock_index].reference_bit!=0){
        physical_page_array[ws_clock_index].reference_bit=0;
        ++ws_clock_index;
        ws_clock_index%=physical_frames;
    }
    get_page_in_disk(disk_page_num,ws_clock_index);
    ++ws_clock_index;
    ws_clock_index%=physical_frames;
}

/* Applies the global Not recently used algorithm. Evaluates pages as 4 classes and replaces the lowest class. */
void NRU_algorithm(int disk_page_num){
    int i;
    for(i=0;i<physical_frames;++i){
        if(physical_page_array[i].reference_bit==0 && physical_page_array[i].modified_bit==0){
            get_page_in_disk(disk_page_num,i);
            return;
        }
    }
    for(i=0;i<physical_frames;++i){
        if(physical_page_array[i].reference_bit==1 && physical_page_array[i].modified_bit==0){
            get_page_in_disk(disk_page_num,i);
            return;
        }
    }
    for(i=0;i<physical_frames;++i){
        if(physical_page_array[i].reference_bit==0 && physical_page_array[i].modified_bit==1){
            get_page_in_disk(disk_page_num,i);
            return;
        }
    }
    for(i=0;i<physical_frames;++i){
        if(physical_page_array[i].reference_bit==1 && physical_page_array[i].modified_bit==1){
            get_page_in_disk(disk_page_num,i);
            return;
        }
    }
}

/* Finds and returns the address of the given virtual memory index. If the address's values are in physical memory, it returns directly. Otherwise, it copies the page from the disk with the page replacement algorithm and takes the value from there. */
int get(unsigned int index, char * tName){
    int ret_val;
    if(strcmp(tName,"index")==0){
        ret_val=virtual_memory[index];
    }
    /* Page fault(Not hits) */
    else if((ret_val=get_physical(index))==-1){
        int disk_page=(int)virtual_memory[index]/(int)frame_size;
        if(page_replacement==NRU){
            NRU_algorithm(disk_page);
        }
        else if(page_replacement==FIFO){
            fifo_algorithm(disk_page);
        }
        else if(page_replacement==SC){
            second_chance_algorithm(disk_page);
        }
        else if(page_replacement==LRU){
            LRU_algorithm(disk_page);
        }
        else if(page_replacement==WSClock){
            WSClock_algorithm(disk_page);
        }
        ret_val=get_physical(index);
        ++page_fault;
    }
    /* Hits */
    else{
        int page_index=is_hold_physical_mem(index);
        physical_page_array[page_index].recent_access_time=get_time_microseconds();
        physical_page_array[page_index].modified_bit=1;
        ++physical_page_array[page_index].lru_used;
    }

    return ret_val;
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
           }      
       } 
   }      
} 

/* Index sort like bubble sort, Unlike other sorts, it sorts virtual memory's indexes (addresses), not its values. */
void index_sort(int* virtual_array,int start_index, int size){ 
   int i, j; 
   for (i = start_index; i < (start_index+size)-1; i++){
       for (j = start_index; j < (start_index+size)-i-1; j++){
           if (get(j,"index") > get(j+1,"index")){
               int temp=virtual_array[j];
               virtual_array[j]=virtual_array[j+1];
               virtual_array[j+1]=temp;
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
        } 
    }
    temp=virtual_array[i+1];
    virtual_array[i+1]=virtual_array[right];
    virtual_array[right]=temp;
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
            } 
            virtual_array[start_index] = value; 

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

/* frees memory */
void free_memory(){
    free(virtual_memory);
    free(physical_memory);
    free(disk);
    free(physical_page_array);
}

/* Finds the optimal page size by changing the frame size in the loop and reinitializing it for each sorting algorithm. */
void find_optimal_page_size(){
    int page_faults[10],i,min_page_fault,min_page_size;
    initialize_memory();
    fill_memory();
    printf("***FINDING OPTIMAL PAGE SIZE:***\n");
    min_page_fault=999999999;
    printf("\n**Merge sort**\n");
    for(i=1;pow(2,i)<=physical_int;++i){
        page_fault=0;
        reinitialize_and_fill_memory(pow(2,i));
        merge_sort(virtual_memory,0,virtual_int-1);
        
        page_faults[i-1]=page_fault;
        if(page_fault<min_page_fault){
            min_page_fault=page_fault;
            min_page_size=(int)pow(2,i);
        }
            
        printf("page fault in %d page size:%d\n",(int)pow(2,i),page_faults[i-1]);
    }
    printf("RESULT:Optimum page size in merge sort:%d\n",min_page_size);

    min_page_fault=999999999;
    printf("\n**Quick sort**\n");
    for(i=1;pow(2,i)<=physical_int;++i){
        page_fault=0;
        reinitialize_and_fill_memory(pow(2,i));
        quick_sort(virtual_memory,0,virtual_int-1);
        
        page_faults[i-1]=page_fault;
        if(page_fault<min_page_fault){
            min_page_fault=page_fault;
            min_page_size=(int)pow(2,i);
        }
        printf("page fault in %d page size:%d\n",(int)pow(2,i),page_faults[i-1]);
    }
    printf("RESULT:Optimum page size in quick sort:%d\n",min_page_size);

    
    min_page_fault=999999999;
    printf("\n**Index sort**\n");
    for(i=1;pow(2,i)<=physical_int;++i){
        page_fault=0;
        reinitialize_and_fill_memory(pow(2,i));
        index_sort(virtual_memory,0,virtual_int-1);
        
        page_faults[i-1]=page_fault;
        if(page_fault<min_page_fault){
            min_page_fault=page_fault;
            min_page_size=(int)pow(2,i);
        }
        printf("page fault in %d page size:%d\n",(int)pow(2,i),page_faults[i-1]);
    }
    printf("RESULT:Optimum page size in index sort:%d\n",min_page_size);

    
    min_page_fault=999999999;
    printf("\n**Bubble sort**\n");
    for(i=1;pow(2,i)<=physical_int;++i){
        page_fault=0;
        reinitialize_and_fill_memory(pow(2,i));
        bubble_sort(virtual_memory,0,virtual_int);
        
        page_faults[i-1]=page_fault;
        if(page_fault<min_page_fault){
            min_page_fault=page_fault;
            min_page_size=(int)pow(2,i);
        }
        printf("page fault in %d page size:%d\n",(int)pow(2,i),page_faults[i-1]);
    }
    printf("RESULT:Optimum page size in bubble sort:%d\n",min_page_size);
}

/* Finds the best page replacement algorithm by changing the frame size in the loop and reinitializing it for each sorting algorithm. */
void find_best_page_replacement(){
    int i,min_page_fault,min_algorithm;
    printf("\n***FINDING BEST PAGE REPLACEMENT ALGORITHM:***\n");
    printf("0->NRU, 1->FIFO, 2->SC 3->LRU 4->WSClock\n");
    fifo_index=0;
    ws_clock_index=0;
    second_chance_index=0;

    min_page_fault=999999999;
    printf("\nFinding best page replacement for merge sort\n");
    for(i=0;i<5;++i){
        page_fault=0;
        page_replacement=i;
        reinitialize_and_fill_memory(pow(2,2));
        merge_sort(virtual_memory,0,virtual_int-1);
        if(page_fault<min_page_fault){
            min_page_fault=page_fault;
            min_algorithm=i;
        }
    }
    printf("RESULT:Best page replacement for merge sort:%d\n",min_algorithm);

    fifo_index=0;
    ws_clock_index=0;
    second_chance_index=0;

    min_page_fault=999999999;
    printf("\nBest page replacement for quick sort\n");
    for(i=0;i<5;++i){
        page_fault=0;
        page_replacement=i;
        reinitialize_and_fill_memory(pow(2,2));
        quick_sort(virtual_memory,0,virtual_int-1);
        if(page_fault<=min_page_fault){
            min_page_fault=page_fault;
            min_algorithm=i;
        }
    }
    printf("RESULT:Best page replacement for quick sort:%d\n",min_algorithm);

    fifo_index=0;
    ws_clock_index=0;
    second_chance_index=0;

    min_page_fault=999999999;
    printf("\nBest page replacement for index sort\n");
    for(i=0;i<5;++i){
        page_fault=0;
        page_replacement=i;
        reinitialize_and_fill_memory(pow(2,2));
        index_sort(virtual_memory,0,virtual_int-1);
        if(page_fault<min_page_fault){
            min_page_fault=page_fault;
            min_algorithm=i;
        }
    }
    printf("RESULT:Best page replacement for index sort:%d\n",min_algorithm);

    fifo_index=0;
    ws_clock_index=0;
    second_chance_index=0;

    min_page_fault=999999999;
    printf("\nBest page replacement for bubble sort\n");
    for(i=0;i<5;++i){
        page_fault=0;
        page_replacement=i;
        reinitialize_and_fill_memory(pow(2,2));
        bubble_sort(virtual_memory,0,virtual_int-1);
        if(page_fault<=min_page_fault){
            min_page_fault=page_fault;
            min_algorithm=i;
        }
    }
    printf("RESULT:Best page replacement for bubble sort:%d\n",min_algorithm);
}


/* Main function */
int main(){
    /* Dummy initialization(these will change in functions) */
    frame_size=pow(2,1);
    virtual_frames=pow(2,11);
    virtual_int=4*1024;
    physical_frames=pow(2,7);
    physical_int=1024;

    /* index and counter initialization */
    fifo_index=0;
    second_chance_index=0;
    ws_clock_index=0;
    lru_counter=0;
    page_replacement=FIFO;

    /* Print infos */
    printf("virtual_int:%d physical_int:%d\n",virtual_int,physical_int);

    /* Find optimal page size */
    find_optimal_page_size();

    /* Find best replacement algorithm */
    find_best_page_replacement();

    /* frees the memories */
    free_memory();

    return 0;
}
