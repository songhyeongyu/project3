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
	struct pte *current_pte;					// page table entry
	struct pd *current_pd;						// page directory
	struct pagetable *current_pagetable = ptbr; // page table bases - resgisters
	int pd_index = vpn / NR_PTES_PER_PAGE;		// page를 모아놓은 것들 index ,an index into the page table = vpn
	int pte_index = vpn % NR_PTES_PER_PAGE;		// page table entry index
	static int cnt = 0;

	// 반대로;
	current_pte = &current_pagetable->pdes[pd_index]->ptes[pte_index]; // 현재 pte
	mapcounts[current_pte->pfn] -= 1;								   // map count를 0으로 조정해주고
	current_pte->pfn = 0;											   // phsical frame number
	current_pte->valid = 0;
	current_pte->rw = ACCESS_NONE;
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
	static int cnt = 0;
	// pte사용은 어떻게?
	//  processes들의 모임을 만들어야 된다.
	//   list head는 -> current(?) -> new로 process를 만들어 주고
	//
	//  switch의 명렁어가 오면  새로운 프로세스를 만들어야 된다 == fork()

	// fork를 시캬줘야되는데 ptbr에 대해서 fork를 시켜준다.
	// 1 . ptbr에 대해서 fork를 시켜주면 총 128개를 돌아야 된다.
	//  나는 vpn값을 모른체로 시작한다.
	// 흠 ...

	// if there is a process with pid in @procces
	
	if (!list_empty(&processes))
	{
		list_for_each_entry(tmp, &processes, list)
		{
			if (pid == tmp->pid)
			{
				
				current = tmp;
				break;
			}
		}
	}

	// if there is not process
	//  malloc 하고
	else
	{
		
		new = malloc(sizeof(struct process));			   // new process의 공간을 확보하고 새로 잡고
		
		for (int i = 0; i < NR_PDES_PER_PAGE; i++)
		{
			if (current_pagetable->pdes[i] == NULL)
			{ // 현재 pagetable이 비어있다? 내가 필요로 하는 pagetable이 아님 new도 null로 초기화
				new_pagetable->pdes[i] = NULL;
				continue;
			}
			else
			{
				new_pagetable->pdes[i] =  malloc(sizeof(struct pagetable) * NR_PTES_PER_PAGE);
			}

			//new에다가 current를 다시 넣어야된다.
			//new_pagetable의 공간은 잡았고 current는 이미 잡혀있다.
			for(int j=0;j<NR_PTES_PER_PAGE;j++)
			{	
				
				new_pagetable = current_pagetable;

				
			}

		}
	}

	//mapcount는 밖에서 진행 안에서 진행하면 3중 for문사용
	for(int i = 0;i<NR_PAGEFRAMES;i++){
		if(mapcounts[i] == '\0'){
			new_pte->pfn = i;
			cnt = i;
			mapcounts[i]++;
		}
	}
	if(cnt > NR_PAGEFRAMES){
		exit(EXIT_FAILURE);
	}

	new->pid = pid;									   // 일단 새로운 pid를 만들엇고 process에 넣어야지
	// current = new;
	// list_replace(&current->list,&new->list);
	// list_add_tail(&current->list, &processes);
	
	
}
