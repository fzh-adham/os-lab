// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.

	if(!(err &FEC_WR)&&(uvpt[PPN(addr)]& PTE_COW))
		panic("page fault!!! page is not writeable\n");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.

	//panic("pgfault not implemented");

	void *Pageaddr ;
	if(0 == sys_page_alloc(0,(void*)PFTEMP,PTE_U|PTE_P|PTE_W))
	{
		Pageaddr = ROUNDDOWN(addr,PGSIZE);
		memmove(PFTEMP, Pageaddr, PGSIZE);
		if(0> sys_page_map(0,PFTEMP,0,Pageaddr,PTE_U|PTE_P|PTE_W))
				panic("Page map at temp address failed");
		if(0> sys_page_unmap(0,PFTEMP))
				panic("Page unmap from temp location failed");
	}
	else
	{
		panic("Page assigning failed when handle page fault");
	}
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	//panic("duppage not implemented");
	int perm = (uvpt[pn]) & PTE_SYSCALL;
	void* addr = (void*)((uint64_t)pn *PGSIZE);

	if(perm & PTE_SHARE){
			if(0 < sys_page_map(0,addr,envid,addr,perm))
				panic("Page alloc with COW  failed.\n");

		}else{
					if((perm & PTE_W || perm & PTE_COW)){

						perm = (perm|PTE_COW)&(~PTE_W);

						if(0 < sys_page_map(0,addr,envid,addr,perm))
							panic("Page alloc with COW  failed.\n");

						if(0 <  sys_page_map(0,addr,0,addr,perm))
							panic("Page alloc with COW  failed.\n");

					}else{
						if(0 < sys_page_map(0,addr,envid,addr,perm))
							panic("Page alloc with COW  failed.\n");
					}
		}
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	//panic("fork not implemented");


	envid_t envid;
	int r;

	uint64_t i;
	uint64_t addr, last;

	set_pgfault_handler(pgfault);
	envid = sys_exofork();


	if(envid < 0)
		panic("\nsys_exofork error: %e\n", envid);
    else if(envid == 0){
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	uint64_t ad = 0;
	for (addr = (uint64_t)USTACKTOP-PGSIZE; addr >=(uint64_t)UTEXT; addr -= PGSIZE){
		if(uvpml4e[VPML4E(addr)]& PTE_P){
			if( uvpde[VPDPE(addr)] & PTE_P){
				if( uvpd[VPD(addr)] & PTE_P){
					if((ad =uvpt[VPN(addr)])& PTE_P){
						duppage(envid, VPN(addr));
						}
					}else{
						addr -= NPDENTRIES*PGSIZE;
				}
			}else{
				addr -= NPDENTRIES*NPDENTRIES*PGSIZE;
			}

		}else{
			addr -= ((VPML4E(addr)+1)<<PML4SHIFT);
		}

	}


	sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE),PTE_P|PTE_U|PTE_W);
	sys_page_alloc(envid, (void*)(USTACKTOP - PGSIZE),PTE_P|PTE_U|PTE_W);
	sys_page_map(envid, (void*)(USTACKTOP - PGSIZE), 0, PFTEMP,PTE_P|PTE_U|PTE_W);

	memmove(PFTEMP, (void*)(USTACKTOP-PGSIZE), PGSIZE);

	sys_page_unmap(0, PFTEMP);
  sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall);
	sys_env_set_status(envid, ENV_RUNNABLE);

	return envid;
}


// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
