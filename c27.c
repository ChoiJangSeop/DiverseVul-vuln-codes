pfm_smpl_buffer_alloc(struct task_struct *task, pfm_context_t *ctx, unsigned long rsize, void **user_vaddr)
{
	struct mm_struct *mm = task->mm;
	struct vm_area_struct *vma = NULL;
	unsigned long size;
	void *smpl_buf;


	/*
	 * the fixed header + requested size and align to page boundary
	 */
	size = PAGE_ALIGN(rsize);

	DPRINT(("sampling buffer rsize=%lu size=%lu bytes\n", rsize, size));

	/*
	 * check requested size to avoid Denial-of-service attacks
	 * XXX: may have to refine this test
	 * Check against address space limit.
	 *
	 * if ((mm->total_vm << PAGE_SHIFT) + len> task->rlim[RLIMIT_AS].rlim_cur)
	 * 	return -ENOMEM;
	 */
	if (size > task->signal->rlim[RLIMIT_MEMLOCK].rlim_cur)
		return -ENOMEM;

	/*
	 * We do the easy to undo allocations first.
 	 *
	 * pfm_rvmalloc(), clears the buffer, so there is no leak
	 */
	smpl_buf = pfm_rvmalloc(size);
	if (smpl_buf == NULL) {
		DPRINT(("Can't allocate sampling buffer\n"));
		return -ENOMEM;
	}

	DPRINT(("smpl_buf @%p\n", smpl_buf));

	/* allocate vma */
	vma = kmem_cache_zalloc(vm_area_cachep, GFP_KERNEL);
	if (!vma) {
		DPRINT(("Cannot allocate vma\n"));
		goto error_kmem;
	}

	/*
	 * partially initialize the vma for the sampling buffer
	 */
	vma->vm_mm	     = mm;
	vma->vm_flags	     = VM_READ| VM_MAYREAD |VM_RESERVED;
	vma->vm_page_prot    = PAGE_READONLY; /* XXX may need to change */

	/*
	 * Now we have everything we need and we can initialize
	 * and connect all the data structures
	 */

	ctx->ctx_smpl_hdr   = smpl_buf;
	ctx->ctx_smpl_size  = size; /* aligned size */

	/*
	 * Let's do the difficult operations next.
	 *
	 * now we atomically find some area in the address space and
	 * remap the buffer in it.
	 */
	down_write(&task->mm->mmap_sem);

	/* find some free area in address space, must have mmap sem held */
	vma->vm_start = pfm_get_unmapped_area(NULL, 0, size, 0, MAP_PRIVATE|MAP_ANONYMOUS, 0);
	if (vma->vm_start == 0UL) {
		DPRINT(("Cannot find unmapped area for size %ld\n", size));
		up_write(&task->mm->mmap_sem);
		goto error;
	}
	vma->vm_end = vma->vm_start + size;
	vma->vm_pgoff = vma->vm_start >> PAGE_SHIFT;

	DPRINT(("aligned size=%ld, hdr=%p mapped @0x%lx\n", size, ctx->ctx_smpl_hdr, vma->vm_start));

	/* can only be applied to current task, need to have the mm semaphore held when called */
	if (pfm_remap_buffer(vma, (unsigned long)smpl_buf, vma->vm_start, size)) {
		DPRINT(("Can't remap buffer\n"));
		up_write(&task->mm->mmap_sem);
		goto error;
	}

	/*
	 * now insert the vma in the vm list for the process, must be
	 * done with mmap lock held
	 */
	insert_vm_struct(mm, vma);

	mm->total_vm  += size >> PAGE_SHIFT;
	vm_stat_account(vma->vm_mm, vma->vm_flags, vma->vm_file,
							vma_pages(vma));
	up_write(&task->mm->mmap_sem);

	/*
	 * keep track of user level virtual address
	 */
	ctx->ctx_smpl_vaddr = (void *)vma->vm_start;
	*(unsigned long *)user_vaddr = vma->vm_start;

	return 0;

error:
	kmem_cache_free(vm_area_cachep, vma);
error_kmem:
	pfm_rvfree(smpl_buf, size);

	return -ENOMEM;
}