static int journal_unmap_buffer(journal_t *journal, struct buffer_head *bh)
{
	transaction_t *transaction;
	struct journal_head *jh;
	int may_free = 1;
	int ret;

	BUFFER_TRACE(bh, "entry");

	/*
	 * It is safe to proceed here without the j_list_lock because the
	 * buffers cannot be stolen by try_to_free_buffers as long as we are
	 * holding the page lock. --sct
	 */

	if (!buffer_jbd(bh))
		goto zap_buffer_unlocked;

	/* OK, we have data buffer in journaled mode */
	write_lock(&journal->j_state_lock);
	jbd_lock_bh_state(bh);
	spin_lock(&journal->j_list_lock);

	jh = jbd2_journal_grab_journal_head(bh);
	if (!jh)
		goto zap_buffer_no_jh;

	/*
	 * We cannot remove the buffer from checkpoint lists until the
	 * transaction adding inode to orphan list (let's call it T)
	 * is committed.  Otherwise if the transaction changing the
	 * buffer would be cleaned from the journal before T is
	 * committed, a crash will cause that the correct contents of
	 * the buffer will be lost.  On the other hand we have to
	 * clear the buffer dirty bit at latest at the moment when the
	 * transaction marking the buffer as freed in the filesystem
	 * structures is committed because from that moment on the
	 * buffer can be reallocated and used by a different page.
	 * Since the block hasn't been freed yet but the inode has
	 * already been added to orphan list, it is safe for us to add
	 * the buffer to BJ_Forget list of the newest transaction.
	 */
	transaction = jh->b_transaction;
	if (transaction == NULL) {
		/* First case: not on any transaction.  If it
		 * has no checkpoint link, then we can zap it:
		 * it's a writeback-mode buffer so we don't care
		 * if it hits disk safely. */
		if (!jh->b_cp_transaction) {
			JBUFFER_TRACE(jh, "not on any transaction: zap");
			goto zap_buffer;
		}

		if (!buffer_dirty(bh)) {
			/* bdflush has written it.  We can drop it now */
			goto zap_buffer;
		}

		/* OK, it must be in the journal but still not
		 * written fully to disk: it's metadata or
		 * journaled data... */

		if (journal->j_running_transaction) {
			/* ... and once the current transaction has
			 * committed, the buffer won't be needed any
			 * longer. */
			JBUFFER_TRACE(jh, "checkpointed: add to BJ_Forget");
			ret = __dispose_buffer(jh,
					journal->j_running_transaction);
			jbd2_journal_put_journal_head(jh);
			spin_unlock(&journal->j_list_lock);
			jbd_unlock_bh_state(bh);
			write_unlock(&journal->j_state_lock);
			return ret;
		} else {
			/* There is no currently-running transaction. So the
			 * orphan record which we wrote for this file must have
			 * passed into commit.  We must attach this buffer to
			 * the committing transaction, if it exists. */
			if (journal->j_committing_transaction) {
				JBUFFER_TRACE(jh, "give to committing trans");
				ret = __dispose_buffer(jh,
					journal->j_committing_transaction);
				jbd2_journal_put_journal_head(jh);
				spin_unlock(&journal->j_list_lock);
				jbd_unlock_bh_state(bh);
				write_unlock(&journal->j_state_lock);
				return ret;
			} else {
				/* The orphan record's transaction has
				 * committed.  We can cleanse this buffer */
				clear_buffer_jbddirty(bh);
				goto zap_buffer;
			}
		}
	} else if (transaction == journal->j_committing_transaction) {
		JBUFFER_TRACE(jh, "on committing transaction");
		/*
		 * The buffer is committing, we simply cannot touch
		 * it. So we just set j_next_transaction to the
		 * running transaction (if there is one) and mark
		 * buffer as freed so that commit code knows it should
		 * clear dirty bits when it is done with the buffer.
		 */
		set_buffer_freed(bh);
		if (journal->j_running_transaction && buffer_jbddirty(bh))
			jh->b_next_transaction = journal->j_running_transaction;
		jbd2_journal_put_journal_head(jh);
		spin_unlock(&journal->j_list_lock);
		jbd_unlock_bh_state(bh);
		write_unlock(&journal->j_state_lock);
		return 0;
	} else {
		/* Good, the buffer belongs to the running transaction.
		 * We are writing our own transaction's data, not any
		 * previous one's, so it is safe to throw it away
		 * (remember that we expect the filesystem to have set
		 * i_size already for this truncate so recovery will not
		 * expose the disk blocks we are discarding here.) */
		J_ASSERT_JH(jh, transaction == journal->j_running_transaction);
		JBUFFER_TRACE(jh, "on running transaction");
		may_free = __dispose_buffer(jh, transaction);
	}

zap_buffer:
	jbd2_journal_put_journal_head(jh);
zap_buffer_no_jh:
	spin_unlock(&journal->j_list_lock);
	jbd_unlock_bh_state(bh);
	write_unlock(&journal->j_state_lock);
zap_buffer_unlocked:
	clear_buffer_dirty(bh);
	J_ASSERT_BH(bh, !buffer_jbddirty(bh));
	clear_buffer_mapped(bh);
	clear_buffer_req(bh);
	clear_buffer_new(bh);
	bh->b_bdev = NULL;
	return may_free;
}