static bool __oom_reap_task_mm(struct task_struct *tsk, struct mm_struct *mm)
{
	struct mmu_gather tlb;
	struct vm_area_struct *vma;
	bool ret = true;

	/*
	 * We have to make sure to not race with the victim exit path
	 * and cause premature new oom victim selection:
	 * __oom_reap_task_mm		exit_mm
	 *   mmget_not_zero
	 *				  mmput
	 *				    atomic_dec_and_test
	 *				  exit_oom_victim
	 *				[...]
	 *				out_of_memory
	 *				  select_bad_process
	 *				    # no TIF_MEMDIE task selects new victim
	 *  unmap_page_range # frees some memory
	 */
	mutex_lock(&oom_lock);

	if (!down_read_trylock(&mm->mmap_sem)) {
		ret = false;
		trace_skip_task_reaping(tsk->pid);
		goto unlock_oom;
	}

	/*
	 * If the mm has invalidate_{start,end}() notifiers that could block,
	 * sleep to give the oom victim some more time.
	 * TODO: we really want to get rid of this ugly hack and make sure that
	 * notifiers cannot block for unbounded amount of time
	 */
	if (mm_has_blockable_invalidate_notifiers(mm)) {
		up_read(&mm->mmap_sem);
		schedule_timeout_idle(HZ);
		goto unlock_oom;
	}

	/*
	 * MMF_OOM_SKIP is set by exit_mmap when the OOM reaper can't
	 * work on the mm anymore. The check for MMF_OOM_SKIP must run
	 * under mmap_sem for reading because it serializes against the
	 * down_write();up_write() cycle in exit_mmap().
	 */
	if (test_bit(MMF_OOM_SKIP, &mm->flags)) {
		up_read(&mm->mmap_sem);
		trace_skip_task_reaping(tsk->pid);
		goto unlock_oom;
	}

	trace_start_task_reaping(tsk->pid);

	/*
	 * Tell all users of get_user/copy_from_user etc... that the content
	 * is no longer stable. No barriers really needed because unmapping
	 * should imply barriers already and the reader would hit a page fault
	 * if it stumbled over a reaped memory.
	 */
	set_bit(MMF_UNSTABLE, &mm->flags);

	for (vma = mm->mmap ; vma; vma = vma->vm_next) {
		if (!can_madv_dontneed_vma(vma))
			continue;

		/*
		 * Only anonymous pages have a good chance to be dropped
		 * without additional steps which we cannot afford as we
		 * are OOM already.
		 *
		 * We do not even care about fs backed pages because all
		 * which are reclaimable have already been reclaimed and
		 * we do not want to block exit_mmap by keeping mm ref
		 * count elevated without a good reason.
		 */
		if (vma_is_anonymous(vma) || !(vma->vm_flags & VM_SHARED)) {
			const unsigned long start = vma->vm_start;
			const unsigned long end = vma->vm_end;

			tlb_gather_mmu(&tlb, mm, start, end);
			mmu_notifier_invalidate_range_start(mm, start, end);
			unmap_page_range(&tlb, vma, start, end, NULL);
			mmu_notifier_invalidate_range_end(mm, start, end);
			tlb_finish_mmu(&tlb, start, end);
		}
	}
	pr_info("oom_reaper: reaped process %d (%s), now anon-rss:%lukB, file-rss:%lukB, shmem-rss:%lukB\n",
			task_pid_nr(tsk), tsk->comm,
			K(get_mm_counter(mm, MM_ANONPAGES)),
			K(get_mm_counter(mm, MM_FILEPAGES)),
			K(get_mm_counter(mm, MM_SHMEMPAGES)));
	up_read(&mm->mmap_sem);

	trace_finish_task_reaping(tsk->pid);
unlock_oom:
	mutex_unlock(&oom_lock);
	return ret;
}