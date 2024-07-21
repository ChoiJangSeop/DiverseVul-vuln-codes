static long vma_compute_subtree_gap(struct vm_area_struct *vma)
{
	unsigned long max, subtree_gap;
	max = vma->vm_start;
	if (vma->vm_prev)
		max -= vma->vm_prev->vm_end;
	if (vma->vm_rb.rb_left) {
		subtree_gap = rb_entry(vma->vm_rb.rb_left,
				struct vm_area_struct, vm_rb)->rb_subtree_gap;
		if (subtree_gap > max)
			max = subtree_gap;
	}
	if (vma->vm_rb.rb_right) {
		subtree_gap = rb_entry(vma->vm_rb.rb_right,
				struct vm_area_struct, vm_rb)->rb_subtree_gap;
		if (subtree_gap > max)
			max = subtree_gap;
	}
	return max;
}