int __init early_fixup_exception(unsigned long *ip)
{
	const struct exception_table_entry *fixup;
	unsigned long new_ip;

	fixup = search_exception_tables(*ip);
	if (fixup) {
		new_ip = ex_fixup_addr(fixup);

		if (fixup->fixup - fixup->insn >= 0x7ffffff0 - 4) {
			/* uaccess handling not supported during early boot */
			return 0;
		}

		*ip = new_ip;
		return 1;
	}

	return 0;
}