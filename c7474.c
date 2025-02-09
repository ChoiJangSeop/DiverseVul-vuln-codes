void uverbs_user_mmap_disassociate(struct ib_uverbs_file *ufile)
{
	struct rdma_umap_priv *priv, *next_priv;

	lockdep_assert_held(&ufile->hw_destroy_rwsem);

	while (1) {
		struct mm_struct *mm = NULL;

		/* Get an arbitrary mm pointer that hasn't been cleaned yet */
		mutex_lock(&ufile->umap_lock);
		while (!list_empty(&ufile->umaps)) {
			int ret;

			priv = list_first_entry(&ufile->umaps,
						struct rdma_umap_priv, list);
			mm = priv->vma->vm_mm;
			ret = mmget_not_zero(mm);
			if (!ret) {
				list_del_init(&priv->list);
				mm = NULL;
				continue;
			}
			break;
		}
		mutex_unlock(&ufile->umap_lock);
		if (!mm)
			return;

		/*
		 * The umap_lock is nested under mmap_sem since it used within
		 * the vma_ops callbacks, so we have to clean the list one mm
		 * at a time to get the lock ordering right. Typically there
		 * will only be one mm, so no big deal.
		 */
		down_write(&mm->mmap_sem);
		mutex_lock(&ufile->umap_lock);
		list_for_each_entry_safe (priv, next_priv, &ufile->umaps,
					  list) {
			struct vm_area_struct *vma = priv->vma;

			if (vma->vm_mm != mm)
				continue;
			list_del_init(&priv->list);

			zap_vma_ptes(vma, vma->vm_start,
				     vma->vm_end - vma->vm_start);
			vma->vm_flags &= ~(VM_SHARED | VM_MAYSHARE);
		}
		mutex_unlock(&ufile->umap_lock);
		up_write(&mm->mmap_sem);
		mmput(mm);
	}
}