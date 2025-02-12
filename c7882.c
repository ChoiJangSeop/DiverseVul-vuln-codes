void snd_info_free_entry(struct snd_info_entry * entry)
{
	struct snd_info_entry *p, *n;

	if (!entry)
		return;
	if (entry->p) {
		mutex_lock(&info_mutex);
		snd_info_disconnect(entry);
		mutex_unlock(&info_mutex);
	}

	/* free all children at first */
	list_for_each_entry_safe(p, n, &entry->children, list)
		snd_info_free_entry(p);

	list_del(&entry->list);
	kfree(entry->name);
	if (entry->private_free)
		entry->private_free(entry);
	kfree(entry);
}