/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//enum to check replacement strategy
typedef enum {
	ran,
	fifo,
	custom
} replacement_strategy; 

//struct to keep track of page faults, disk reads, and disk writes
struct counter {
	int page_faults;
	int disk_reads;
	int disk_writes;
};

//Structure to keep track of when frames were added
typedef struct Frame {
	int page_number;
	int dirty;
	int time;
} Frame;

//Global variables 
struct counter count;
Frame *frames_list;
struct disk *disk;
replacement_strategy replace;

//Returns the frame number of a free frame. If -1 is returned, there are no free frames
int get_free_frame(struct page_table *pt) {
	int i;
	for(i=0; i < page_table_get_nframes(pt); i++){
		if(frames_list[i].page_number == -1){
			return i;
		}
	}
	return -1;
}

//Gets the oldest frame in the frames_list. Used for FIFO
int get_oldest_frame(struct page_table *pt) {
	int i;
	int min = frames_list[0].time;
	int frame = 0;
	for(i=1; i < page_table_get_nframes(pt); i++){
		if(frames_list[i].time < min){
			frame = i;
			min = frames_list[i].time;
		}
	}
	return frame;
}

//Get the newest frame in the frames_list. Used for LIFO (Custom)
int get_newest_frame(struct page_table *pt) {
	int i;
	int max = frames_list[0].time;
	int frame = 0;
	for(i=1; i < page_table_get_nframes(pt); i++){
		if(frames_list[i].time > max){
			frame = i;
			max = frames_list[i].time;
		}
	}
	return frame;
}

//Gets the oldest frame that isn't dirty
int get_oldest_clean_frame(struct page_table *pt) {
	int i;
	int frame = -1;
	int min = count.page_faults;
	for(i=0; i < page_table_get_nframes(pt); i++){
		if((frames_list[i].time < min) && (!frames_list[i].dirty)){
			frame = i;
			min = frames_list[i].time;
		}
	}
	//printf("Frame to be replaced is #%d\n", frame);
	return frame;
}

//Loads a page into a particular frame
void set_frame(struct page_table *pt, int page, int frame) {
	count.disk_reads++;
	frames_list[frame].page_number = page;
	frames_list[frame].dirty = 0;
	frames_list[frame].time = count.page_faults; //This is used for FIFO strategy. Time is represented as the increasing number of page faults. Since there can only be one swap per page fault, the smallest "time" value currently in the frames_list will be the first in, and therefore should be replaced
	page_table_set_entry(pt, page, frame, PROT_READ);
	char *physmem = page_table_get_physmem(pt);
	disk_read(disk, page, &physmem[frame * BLOCK_SIZE]);
}

//Swaps one page into a fram and removes the other page from that frame
void swap_pages(struct page_table *pt, int old_page, int new_page, int frame) {
	set_frame(pt, new_page, frame);
	page_table_set_entry(pt, old_page, frame, 0);
}

//Sets the write permission without modifying the page table for frames list
void set_write_permission(struct page_table *pt, int page, int frame) {
	frames_list[frame].dirty = 1;
	page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
}

//Writes a particular page back to disk
void write_to_disk(struct page_table *pt, int page, int frame) {
	count.disk_writes++;
	char *physmem = page_table_get_physmem(pt);
	disk_write(disk, page, &physmem[frame * BLOCK_SIZE]);
}

//Writes every page back to disk, then resets their dirty bits
void write_all_to_disk(struct page_table *pt) {
	int frame, page;
	for(frame=0; frame < page_table_get_nframes(pt); frame++){
		page = frames_list[frame].page_number;
		write_to_disk(pt, page, frame);

		//unset the dirty bits
		frames_list[frame].dirty = 0;
		page_table_set_entry(pt, page, frame, PROT_READ);
	}
}

//Define the three replacement strategies, all of which will call the following function once they decide on a frame to replace
void replace_page(struct page_table *pt, int new_page, int frame) {
	//Check if the page in that frame is dirty
	int old_page = frames_list[frame].page_number;
	if(frames_list[frame].dirty) {
		//page is dirty, write back to disk
		write_to_disk(pt, old_page, frame);
	}
	//Swap new page in
	swap_pages(pt, old_page, new_page, frame);
}

//Random replace chooses a random frame and swaps it out accordingly
void random_replace(struct page_table *pt, int new_page) {
	//printf("Random replacement\n");

	//Get a random frame to replace
	int max_frames = page_table_get_nframes(pt);
	int frame = lrand48() % max_frames;

	//replace the chosen frame
	replace_page(pt, new_page, frame);
}

//Replace the frams that was put in first, i.e., the oldest frame
void fifo_replace(struct page_table *pt, int new_page) {
	//printf("FIFO replacement\n");

	//Get the frame that is the oldest
	int frame = get_oldest_frame(pt);

	//replace the chosen frame
	replace_page(pt, new_page, frame);
}

//This is modified FIFO, where it doesn't write dirty pages back to disk unless it has to
void custom_replace(struct page_table *pt, int new_page) {
	//printf("Custom replacement\n");

	//Get the oldest clean frame
	int frame = get_oldest_clean_frame(pt);
	if(frame == -1){
		//printf("All frames are dirty, write all back to disk\n");
		write_all_to_disk(pt);

		//Since all frames are clean, use regular FIFO here
		frame = get_oldest_frame(pt);
	}

	//replace the chosen frame
	replace_page(pt, new_page, frame);
}

//Default handler for page faults. Calls different functions 
//based on the current status of the page table
void page_fault_handler( struct page_table *pt, int page )
{
	//printf("page fault on page #%d\n",page);
	count.page_faults++;
	
	//Check if the page has been written
	int current_frame;
	int bits;
	page_table_get_entry(pt, page, &current_frame, &bits);
	if(bits & PROT_WRITE) {
		//printf("WRITE permission\n");
	} else if(bits & PROT_READ) {
		//The page is in memory, but does not yet have write permission
		//printf("Setting WRITE for frame #%d\n", current_frame);
		set_write_permission(pt, page, current_frame);
	} else if(bits & PROT_EXEC) {
		//printf("EXEC permission\n");
	} else {
		//printf("NO permission\n");

		//Try to get a free frame to put the page in
		int new_frame = get_free_frame(pt);
		if(new_frame != -1){
			//printf("Setting READ for frame #%d\n", new_frame);
			set_frame(pt, page, new_frame);
		} else {
			//printf("No free frames\n");
			switch(replace) {
				case ran: 
					random_replace(pt, page);
					break;
				case fifo: 
					fifo_replace(pt, page);
					break;
				case custom: 
					custom_replace(pt, page);
					break;
			}
		}
	}

}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	if(npages <= 0){
		printf("error: must have at least 1 page\n");
		exit(1);
	}
	if(nframes <= 0){
		printf("error: must have at least 1 frame\n");
		exit(1);
	}
	const char *replacement = argv[3];
	const char *program = argv[4];

	//free frames list keeps track of the free frames in memory
	frames_list = malloc(sizeof(Frame)*nframes);
	int i;
	for(i = 0; i<nframes; i++){
		frames_list[i].page_number = -1;
		frames_list[i].dirty = 0;
		frames_list[i].time = 0;
	}

	//initialize the counts
	count.page_faults = 0;
	count.disk_reads = 0;
	count.disk_writes = 0;

	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}


	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	char * virtmem = page_table_get_virtmem(pt);

	if(!strcmp(replacement,"rand")) {
		replace = ran;
	} else if(!strcmp(replacement,"fifo")) {
		replace = fifo;
	} else if(!strcmp(replacement,"custom")) {
		replace = custom;
	} else {
		fprintf(stderr,"unknown replacement strategy: %s\n",argv[3]);
		exit(1);
	}

	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[4]);
	}

	//printf("\nTotal Page Faults: %d\n", count.page_faults);
	//printf("Total Disk Reads: %d\n", count.disk_reads);
	//printf("Total Disk Writes: %d\n", count.disk_writes);
	printf("%d,%d,%d,%d\n", nframes, count.page_faults, count.disk_reads, count.disk_writes);

	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
