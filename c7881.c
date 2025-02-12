snd_info_create_entry(const char *name, struct snd_info_entry *parent,
		      struct module *module)
{
	struct snd_info_entry *entry;
	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (entry == NULL)
		return NULL;
	entry->name = kstrdup(name, GFP_KERNEL);
	if (entry->name == NULL) {
		kfree(entry);
		return NULL;
	}
	entry->mode = S_IFREG | 0444;
	entry->content = SNDRV_INFO_CONTENT_TEXT;
	mutex_init(&entry->access);
	INIT_LIST_HEAD(&entry->children);
	INIT_LIST_HEAD(&entry->list);
	entry->parent = parent;
	entry->module = module;
	if (parent)
		list_add_tail(&entry->list, &parent->children);
	return entry;
}