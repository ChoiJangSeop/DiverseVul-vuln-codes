static int match_entries(struct entry *entry1, struct entry *entry2)
{
	return entry1->timer       == entry2->timer	  &&
	       entry1->start_func  == entry2->start_func  &&
	       entry1->expire_func == entry2->expire_func &&
	       entry1->pid	   == entry2->pid;
}