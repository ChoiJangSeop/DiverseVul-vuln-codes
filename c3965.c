static int parse_dos_extended(blkid_probe pr, blkid_parttable tab,
		uint32_t ex_start, uint32_t ex_size, int ssf)
{
	blkid_partlist ls = blkid_probe_get_partlist(pr);
	uint32_t cur_start = ex_start, cur_size = ex_size;
	unsigned char *data;
	int ct_nodata = 0;	/* count ext.partitions without data partitions */
	int i;

	while (1) {
		struct dos_partition *p, *p0;
		uint32_t start, size;

		if (++ct_nodata > 100)
			return BLKID_PROBE_OK;
		data = blkid_probe_get_sector(pr, cur_start);
		if (!data) {
			if (errno)
				return -errno;
			goto leave;	/* malformed partition? */
		}

		if (!mbr_is_valid_magic(data))
			goto leave;

		p0 = mbr_get_partition(data, 0);

		/* Usually, the first entry is the real data partition,
		 * the 2nd entry is the next extended partition, or empty,
		 * and the 3rd and 4th entries are unused.
		 * However, DRDOS sometimes has the extended partition as
		 * the first entry (when the data partition is empty),
		 * and OS/2 seems to use all four entries.
		 * -- Linux kernel fs/partitions/dos.c
		 *
		 * See also http://en.wikipedia.org/wiki/Extended_boot_record
		 */

		/* Parse data partition */
		for (p = p0, i = 0; i < 4; i++, p++) {
			uint32_t abs_start;
			blkid_partition par;

			/* the start is relative to the parental ext.partition */
			start = dos_partition_get_start(p) * ssf;
			size = dos_partition_get_size(p) * ssf;
			abs_start = cur_start + start;	/* absolute start */

			if (!size || is_extended(p))
				continue;
			if (i >= 2) {
				/* extra checks to detect real data on
				 * 3rd and 4th entries */
				if (start + size > cur_size)
					continue;
				if (abs_start < ex_start)
					continue;
				if (abs_start + size > ex_start + ex_size)
					continue;
			}

			par = blkid_partlist_add_partition(ls, tab, abs_start, size);
			if (!par)
				return -ENOMEM;

			blkid_partition_set_type(par, p->sys_ind);
			blkid_partition_set_flags(par, p->boot_ind);
			blkid_partition_gen_uuid(par);
			ct_nodata = 0;
		}
		/* The first nested ext.partition should be a link to the next
		 * logical partition. Everything other (recursive ext.partitions)
		 * is junk.
		 */
		for (p = p0, i = 0; i < 4; i++, p++) {
			start = dos_partition_get_start(p) * ssf;
			size = dos_partition_get_size(p) * ssf;

			if (size && is_extended(p))
				break;
		}
		if (i == 4)
			goto leave;

		cur_start = ex_start + start;
		cur_size = size;
	}
leave:
	return BLKID_PROBE_OK;
}