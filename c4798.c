static void reset_entries(void)
{
	nr_entries = 0;
	memset(entries, 0, sizeof(entries));
	memset(tstat_hash_table, 0, sizeof(tstat_hash_table));
	atomic_set(&overflow_count, 0);
}