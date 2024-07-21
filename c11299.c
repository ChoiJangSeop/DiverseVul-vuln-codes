static int FNAME(cmpxchg_gpte)(struct kvm_vcpu *vcpu, struct kvm_mmu *mmu,
			       pt_element_t __user *ptep_user, unsigned index,
			       pt_element_t orig_pte, pt_element_t new_pte)
{
	int npages;
	pt_element_t ret;
	pt_element_t *table;
	struct page *page;

	npages = get_user_pages_fast((unsigned long)ptep_user, 1, FOLL_WRITE, &page);
	if (likely(npages == 1)) {
		table = kmap_atomic(page);
		ret = CMPXCHG(&table[index], orig_pte, new_pte);
		kunmap_atomic(table);

		kvm_release_page_dirty(page);
	} else {
		struct vm_area_struct *vma;
		unsigned long vaddr = (unsigned long)ptep_user & PAGE_MASK;
		unsigned long pfn;
		unsigned long paddr;

		mmap_read_lock(current->mm);
		vma = find_vma_intersection(current->mm, vaddr, vaddr + PAGE_SIZE);
		if (!vma || !(vma->vm_flags & VM_PFNMAP)) {
			mmap_read_unlock(current->mm);
			return -EFAULT;
		}
		pfn = ((vaddr - vma->vm_start) >> PAGE_SHIFT) + vma->vm_pgoff;
		paddr = pfn << PAGE_SHIFT;
		table = memremap(paddr, PAGE_SIZE, MEMREMAP_WB);
		if (!table) {
			mmap_read_unlock(current->mm);
			return -EFAULT;
		}
		ret = CMPXCHG(&table[index], orig_pte, new_pte);
		memunmap(table);
		mmap_read_unlock(current->mm);
	}

	return (ret != orig_pte);
}