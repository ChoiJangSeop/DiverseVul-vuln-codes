void uprobe_dup_mmap(struct mm_struct *oldmm, struct mm_struct *newmm)
{
	newmm->uprobes_state.xol_area = NULL;

	if (test_bit(MMF_HAS_UPROBES, &oldmm->flags)) {
		set_bit(MMF_HAS_UPROBES, &newmm->flags);
		/* unconditionally, dup_mmap() skips VM_DONTCOPY vmas */
		set_bit(MMF_RECALC_UPROBES, &newmm->flags);
	}
}