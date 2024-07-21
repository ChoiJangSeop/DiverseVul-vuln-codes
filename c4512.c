void sort_extable(struct exception_table_entry *start,
		  struct exception_table_entry *finish)
{
	struct exception_table_entry *p;
	int i;

	/* Convert all entries to being relative to the start of the section */
	i = 0;
	for (p = start; p < finish; p++) {
		p->insn += i;
		i += 4;
		p->fixup += i;
		i += 4;
	}

	sort(start, finish - start, sizeof(struct exception_table_entry),
	     cmp_ex, NULL);

	/* Denormalize all entries */
	i = 0;
	for (p = start; p < finish; p++) {
		p->insn -= i;
		i += 4;
		p->fixup -= i;
		i += 4;
	}
}