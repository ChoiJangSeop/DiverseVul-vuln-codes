static void queue_delete(struct snd_seq_queue *q)
{
	/* stop and release the timer */
	snd_seq_timer_stop(q->timer);
	snd_seq_timer_close(q);
	/* wait until access free */
	snd_use_lock_sync(&q->use_lock);
	/* release resources... */
	snd_seq_prioq_delete(&q->tickq);
	snd_seq_prioq_delete(&q->timeq);
	snd_seq_timer_delete(&q->timer);

	kfree(q);
}