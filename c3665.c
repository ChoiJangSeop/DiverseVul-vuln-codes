static int ext4_dax_pmd_fault(struct vm_area_struct *vma, unsigned long addr,
						pmd_t *pmd, unsigned int flags)
{
	int result;
	handle_t *handle = NULL;
	struct inode *inode = file_inode(vma->vm_file);
	struct super_block *sb = inode->i_sb;
	bool write = flags & FAULT_FLAG_WRITE;

	if (write) {
		sb_start_pagefault(sb);
		file_update_time(vma->vm_file);
		handle = ext4_journal_start_sb(sb, EXT4_HT_WRITE_PAGE,
				ext4_chunk_trans_blocks(inode,
							PMD_SIZE / PAGE_SIZE));
	}

	if (IS_ERR(handle))
		result = VM_FAULT_SIGBUS;
	else
		result = __dax_pmd_fault(vma, addr, pmd, flags,
				ext4_get_block_dax, ext4_end_io_unwritten);

	if (write) {
		if (!IS_ERR(handle))
			ext4_journal_stop(handle);
		sb_end_pagefault(sb);
	}

	return result;
}