static int ext4_dax_mkwrite(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	return dax_mkwrite(vma, vmf, ext4_get_block_dax,
				ext4_end_io_unwritten);
}