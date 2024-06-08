/**********************************************************************
 * Copyright (c) 2020-2024
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "list_head.h"
#include "vm.h"

/**
 * Ready queue of the system
 */
extern struct list_head processes;

/**
 * Currently running process
 */
extern struct process *current;

/**
 * Page Table Base Register that MMU will walk through for address translation
 */
extern struct pagetable *ptbr; // 밖에서 관리하는 놈

/**
 * TLB of the system.
 */
extern struct tlb_entry tlb[];

/**
 * The number of mappings for each page frame. Can be used to determine how
 * many processes are using the page frames.
 */
extern unsigned int mapcounts[];
// Switch_count
static int a = 0;
int switch_cnt()
{
	a++;
	return a;
}

/**
 * lookup_tlb(@vpn, @rw, @pfn)
 *
 * DESCRIPTION
 *   Translate @vpn of the current process through TLB. DO NOT make your own
 *   data structure for TLB, but should use the defined @tlb data structure
 *   to translate. If the requested VPN exists in the TLB and it has the same
 *   rw flag, return true with @pfn is set to its PFN. Otherwise, return false.
 *   The framework calls this function when needed, so do not call
 *   this function manually.
 *
 * RETURN
 *   Return true if the translation is cached in the TLB.
 *   Return false otherwise
 */
bool lookup_tlb(unsigned int vpn, unsigned int rw, unsigned int *pfn)
{ // vpn이 존재한다면 tlb에 -> vpn을 pfn으로 바꿔라?

	return true;
}

/**
 * insert_tlb(@vpn, @rw, @pfn)
 *
 * DESCRIPTION
 *   Insert the mapping from @vpn to @pfn for @rw into the TLB. The framework will
 *   call this function when required, so no need to call this function manually.
 *   Note that if there exists an entry for @vpn already, just update it accordingly
 *   rather than removing it or creating a new entry.
 *   Also, in the current simulator, TLB is big enough to cache all the entries of
 *   the current page table, so don't worry about TLB entry eviction. ;-)
 */
void insert_tlb(unsigned int vpn, unsigned int rw, unsigned int pfn)
{
}

/**
 * alloc_page(@vpn, @rw)
 *
 * DESCRIPTION
 *   Allocate a page frame that is not allocated to any process, and map it
 *   to @vpn. When the system has multiple free pages, this function should
 *   allocate the page frame with the **smallest pfn**.
 *   You may construct the page table of the @current process. When the page
 *   is allocated with ACCESS_WRITE flag, the page may be later accessed for writes.
 *   However, the pages populated with ACCESS_READ should not be accessible with
 *   ACCESS_WRITE accesses.
 *
 * RETURN
 *   Return allocated page frame number.
 *   Return -1 if all page frames are allocated.
 */
unsigned int alloc_page(unsigned int vpn, unsigned int rw)
{

	// vpn은 내가 입력하는 숫자
	// pfn 2-levelv
	//  어떤 곳에서도 할당되지 않은 pageframe을 가지는 process -> vpn이랑 mapping
	struct pte *current_pte;					// page table entry -> 16개
	struct pd *current_pd;						// page directory -> 4개
	struct pagetable *current_pagetable = ptbr; // page table bases - resgisters
	int pd_index = vpn / NR_PTES_PER_PAGE;		// page를 모아놓은 것들 index ,an index into the page table = vpn
	int pte_index = vpn % NR_PTES_PER_PAGE;		// page table entry index
	static int cnt = 0;

	// pd_index를 일단 먼저 alloc시켜준다. 1. pd_index가 비어있다면(?)
	if (current_pagetable->pdes[pd_index] == NULL)
	{
		// 16개의 pagetable entry를 malloc해줘야된다.
		current_pagetable->pdes[pd_index] = malloc(sizeof(struct pagetable) * NR_PTES_PER_PAGE); // 1개당 총 16개의 pde생성
	}

	/*여기까지가 1번째줄 */
	// page table enrty setting
	current_pte = &current_pagetable->pdes[pd_index]->ptes[pte_index]; // 현재 pte는 pde -> pte[pte_index에 정의]
	// vaild bit도 바꿔줘야된다. 1 = vaild 0 = invalid
	current_pte->valid = 1;
	current_pte->rw = rw;
	current_pte->private = rw; // read-write fork를 위해서 생성 -> 애를 어떻게 넘겨주지?
	// rw도 바꿔준다. rw가 write -> write
	// rw -> 3 ,w -> 3 , r -> 1

	/*여기까지가 2번째 문단.*/

	// 가장 작은 pfn을 찾아야 된다. pfn은 아직 안건드렸다. 가장 작은 pfn을 찾는방법은?
	// NR_PAGEFRAMES -> 이중에서 찾으면 된다 만약 이 값을 넘는다면? return -1?
	// 16개당 page128개의 frame? -> 실제 frame갯수가 128개 이것 보다 넘어가면 당연히 return -1;
	for (int i = 0; i < NR_PAGEFRAMES; i++) // 실제 frame과 mapping시켜주는 과정.
	{
		if (mapcounts[i] == '\0')
		{
			current_pte->pfn = i;
			// printf("mapcounts[i]1 :%d\n",mapcounts[i]);
			mapcounts[i]++;
			// printf("i : %d\n",i);
			// printf("mapcounts[i]: %d\n",mapcounts[i]);
			cnt = i;
			break;
		}
	}

	if (cnt >= NR_PAGEFRAMES)
	{
		return -1;
	}

	return cnt;
}

/**
 * free_page(@vpn)
 *
 * DESCRIPTION
 *   Deallocate the page from the current processor. Make sure that the fields
 *   for the corresponding PTE (valid, rw, pfn) is set @false or 0.
 *   Also, consider the case when a page is shared by two processes,
 *   and one process is about to free the page. Also, think about TLB as well ;-)
 */

void free_page(unsigned int vpn)
{
	struct pte *current_pte; // page table entry
	struct pagetable *current_pagetable = ptbr;
	int pd_index = vpn / NR_PTES_PER_PAGE;	// page를 모아놓은 것들 index ,an index into the page table = vpn
	int pte_index = vpn % NR_PTES_PER_PAGE; // page table entry index

	// 반대로 이게 일단 하나만 pagetable을 해제한다.;
	current_pte = &current_pagetable->pdes[pd_index]->ptes[pte_index];
	mapcounts[current_pte->pfn]--;
	current_pte->rw = ACCESS_NONE;
	current_pte->valid = 0;
	current_pte->pfn = 0;

	// free 0를 하면 mapping된 모든 pfn을 해제해야 된다?
	//'free' 명령은 VPN에 매핑된 페이지의 할당을 해제하는 것입니다.
	// 해제된 VPN에 대한 후속 액세스가 MMU에 의해 거부되도록 페이지 테이블을 설정해야 합니다.  -> 0에 대해서 계속 거절?
	// 쓰기 중 복사 기능을 사용하여 `free` 명령을 올바르게 처리하려면 대상 페이지 프레임이 2 이상으로 매핑되는 경우를 고려해야 합니다.

	// fork하고 나서 문제가 된다. -> process 1이 새로 쓰고 싶으면
}

/**
 * handle_page_fault()
 *
 * DESCRIPTION
 *   Handle the page fault for accessing @vpn for @rw. This function is called
 *   by the framework when the __translate() for @vpn fails. This implies;
 *   0. page directory is invalid
 *   1. pte is invalid
 *   2. pte is not writable but @rw is for write
 *   This function should identify the situation, and do the copy-on-write if
 *   necessary.
 *
 * RETURN
 *   @true on successful fault handling
 *   @false otherwise
 */
bool handle_page_fault(unsigned int vpn, unsigned int rw)
{
	struct pte *current_pte; // page table entry
	struct pte_directory *current_pte_directory;
	int pd_index = vpn / NR_PTES_PER_PAGE;		// page를 모아놓은 것들 index ,an index into the page table = vpn
	int pte_index = vpn % NR_PTES_PER_PAGE;
	unsigned int new_pfn = 0;
	current_pte_directory = current->pagetable.pdes[pd_index]; //현재 process의 pagetqble의 page directory
	current_pte = &current->pagetable.pdes[pd_index]->ptes[pte_index];
	// if(!current_pd) return true; //fault 발생 0
	// if(current_pte->valid == 0) return true; //fault발생 1
	//pd가 invaild면....
	if (current_pte_directory == NULL)
	{	
		return false;
	}
	//pte가 invaild이면 ?
	if (current_pte->valid == 0)
	{	
		current_pte->valid = 1; //vaild로 바꾸고
		mapcounts[current_pte->pfn]--;
		return true;
	}
	// pte에서 wirte x rw는 가능할때 -> write가능하게해라
	if (current_pte->rw != current_pte->private)
	{	
		if(rw == ACCESS_WRITE){
			rw = ACCESS_WRITE + 0x01;
		}		

		// 나는 이제부터 write를 할거에요 부모님이 주신 write에다가 새로운 write를 할ㄱ거에요
		// // 나 새로운 pfn내놔!!!!!!!!!! -> 젤 작은 pfn 할당 새로 alloc하면 link가 이상해짐
		
		mapcounts[current_pte->pfn]--;
		new_pfn = alloc_page(vpn,rw);

		return true;
	}

	
	// pte가 쓸 수 없는데 쓰려고 할 때 -> fault를 내라

	return false;
}

/**
 * switch_process()
 *
 * DESCRIPTION
 *   If there is a process with @pid in @processes, switch to the process.
 *   The @current process at the moment should be put into the @processes
 *   list, and @current should be replaced to the requested process.
 *   Make sure that the next process is unlinked from the @processes, and
 *   @ptbr is set properly.
 *
 *   If there is no process with @pid in the @processes list, fork a process
 *   from the @current. This implies the forked child process should have
 *   the identical page table entry 'values' to its parent's (i.e., @current)
 *   page table.
 *   To implement the copy-on-write feature, you should manipulate the writable
 *   bit in PTE and mapcounts for shared pages. You may use pte->private for
 *   storing some useful information :-)
 */

void switch_process(unsigned int pid)
{
	struct process *new = NULL;
	struct process *tmp;
	struct pagetable *current_pagetable = ptbr;
	struct pagetable *new_pagetable;
	struct pte *new_pte;
	struct pte *current_pte;
	int cnt = 0;
	// pte사용은 어떻게?
	//  processes들의 모임을 만들어야 된다.
	//   list head는 -> current(?) -> new로 process를 만들어 주고
	//  switch의 명렁어가 오면  새로운 프로세스를 만들어야 된다 == fork()

	// fork를 시캬줘야되는데 ptbr에 대해서 fork를 시켜준다.

	// if there is a process with pid in @procces
	// frame 128개 <->  pd -> pt

	list_for_each_entry(tmp, &processes, list)
	{
		if (pid == tmp->pid)
		{

			current = tmp;
			ptbr = &tmp->pagetable;
			list_add_tail(&current->list, &processes);
			list_del_init(&tmp->list);
			break;
		}
	}

	// if there is not process
	//  malloc 하고 //listhead 는 0
	if (new == NULL) // 1개이상이 processes에 존재하면 오류가 난다.
	{
		new = (struct process *)malloc(sizeof(struct process)); // new process의 공간을 확보하고 새로 잡고

		for (int i = 0; i < NR_PTES_PER_PAGE; i++)
		{
			if (current->pagetable.pdes[i] == NULL) // 현재 current.pagetable pde[i]가 없으면 멈춘다. 왜냐하면 fork할게 없기 때문이다.
			{

				new->pagetable.pdes[i] = NULL;

				break;
			}
			else
			{
				new->pagetable.pdes[i] = malloc(sizeof(struct pagetable) * NR_PTES_PER_PAGE);
				for (int j = 0; j < NR_PTES_PER_PAGE; j++) // pte의 개수만큼 돌린다.
				{

					current_pte = &current_pagetable->pdes[i]->ptes[j];
					new_pte = current_pte;
					// // write가 되면 write가 되고 read, write가 되면 write가 안된다?
					if (current_pte->valid == 1)
					{
						new_pte->valid = 1;
						mapcounts[current_pte->pfn]++;
					}
					if(current_pte->rw == ACCESS_READ || current_pte->rw == ACCESS_WRITE + 0x01){
						new_pte->rw = ACCESS_READ;
					}

					// rw는 read만 가능 , write기능은 사용 안됨, read -> read

					new->pagetable.pdes[i]->ptes[j] = *new_pte; // 새로운 page table의 entry;
				}
			}
		}
		list_add_tail(&new->list, &processes);
		current = new;
		ptbr = &new->pagetable;
		current->pid = pid;
	}

	// mapcount를 조정해야되는데 switch 1번될때마다 다 1씩 올려줘야 되는거 아닌가?
}