static int snd_hrtimer_stop(struct snd_timer *t)
{
	struct snd_hrtimer *stime = t->private_data;
	atomic_set(&stime->running, 0);
	return 0;
}