static void udf_pc_to_char(struct super_block *sb, unsigned char *from,
			   int fromlen, unsigned char *to)
{
	struct pathComponent *pc;
	int elen = 0;
	unsigned char *p = to;

	while (elen < fromlen) {
		pc = (struct pathComponent *)(from + elen);
		switch (pc->componentType) {
		case 1:
			/*
			 * Symlink points to some place which should be agreed
 			 * upon between originator and receiver of the media. Ignore.
			 */
			if (pc->lengthComponentIdent > 0)
				break;
			/* Fall through */
		case 2:
			p = to;
			*p++ = '/';
			break;
		case 3:
			memcpy(p, "../", 3);
			p += 3;
			break;
		case 4:
			memcpy(p, "./", 2);
			p += 2;
			/* that would be . - just ignore */
			break;
		case 5:
			p += udf_get_filename(sb, pc->componentIdent, p,
					      pc->lengthComponentIdent);
			*p++ = '/';
			break;
		}
		elen += sizeof(struct pathComponent) + pc->lengthComponentIdent;
	}
	if (p > to + 1)
		p[-1] = '\0';
	else
		p[0] = '\0';
}