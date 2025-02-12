bool __net_get_random_once(void *buf, int nbytes, bool *done,
			   struct static_key *done_key)
{
	static DEFINE_SPINLOCK(lock);
	unsigned long flags;

	spin_lock_irqsave(&lock, flags);
	if (*done) {
		spin_unlock_irqrestore(&lock, flags);
		return false;
	}

	get_random_bytes(buf, nbytes);
	*done = true;
	spin_unlock_irqrestore(&lock, flags);

	__net_random_once_disable_jump(done_key);

	return true;
}