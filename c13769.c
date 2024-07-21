void snd_pcm_detach_substream(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime;

	if (PCM_RUNTIME_CHECK(substream))
		return;
	runtime = substream->runtime;
	if (runtime->private_free != NULL)
		runtime->private_free(runtime);
	free_pages_exact(runtime->status,
		       PAGE_ALIGN(sizeof(struct snd_pcm_mmap_status)));
	free_pages_exact(runtime->control,
		       PAGE_ALIGN(sizeof(struct snd_pcm_mmap_control)));
	kfree(runtime->hw_constraints.rules);
	/* Avoid concurrent access to runtime via PCM timer interface */
	if (substream->timer) {
		spin_lock_irq(&substream->timer->lock);
		substream->runtime = NULL;
		spin_unlock_irq(&substream->timer->lock);
	} else {
		substream->runtime = NULL;
	}
	kfree(runtime);
	put_pid(substream->pid);
	substream->pid = NULL;
	substream->pstr->substream_opened--;
}