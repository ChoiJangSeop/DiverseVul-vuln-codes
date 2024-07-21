static void __attribute__((__noreturn__)) usage(void)
{
	FILE *out = stdout;
	size_t i;

	fputs(USAGE_HEADER, out);
	fprintf(out, _(
	" %1$s [options]\n"
	" %1$s [options] <device> | <mountpoint>\n"
	" %1$s [options] <device> <mountpoint>\n"
	" %1$s [options] [--source <device>] [--target <path> | --mountpoint <dir>]\n"),
		program_invocation_short_name);

	fputs(USAGE_SEPARATOR, out);
	fputs(_("Find a (mounted) filesystem.\n"), out);

	fputs(USAGE_OPTIONS, out);
	fputs(_(" -s, --fstab            search in static table of filesystems\n"), out);
	fputs(_(" -m, --mtab             search in table of mounted filesystems\n"
		"                          (includes user space mount options)\n"), out);
	fputs(_(" -k, --kernel           search in kernel table of mounted\n"
		"                          filesystems (default)\n"), out);
	fputc('\n', out);
	fputs(_(" -p, --poll[=<list>]    monitor changes in table of mounted filesystems\n"), out);
	fputs(_(" -w, --timeout <num>    upper limit in milliseconds that --poll will block\n"), out);
	fputc('\n', out);

	fputs(_(" -A, --all              disable all built-in filters, print all filesystems\n"), out);
	fputs(_(" -a, --ascii            use ASCII chars for tree formatting\n"), out);
	fputs(_(" -b, --bytes            print sizes in bytes rather than in human readable format\n"), out);
	fputs(_(" -C, --nocanonicalize   don't canonicalize when comparing paths\n"), out);
	fputs(_(" -c, --canonicalize     canonicalize printed paths\n"), out);
	fputs(_("     --deleted          print filesystems with mountpoint marked as deleted\n"), out);
	fputs(_(" -D, --df               imitate the output of df(1)\n"), out);
	fputs(_(" -d, --direction <word> direction of search, 'forward' or 'backward'\n"), out);
	fputs(_(" -e, --evaluate         convert tags (LABEL,UUID,PARTUUID,PARTLABEL) \n"
	        "                          to device names\n"), out);
	fputs(_(" -F, --tab-file <path>  alternative file for -s, -m or -k options\n"), out);
	fputs(_(" -f, --first-only       print the first found filesystem only\n"), out);
	fputs(_(" -i, --invert           invert the sense of matching\n"), out);
	fputs(_(" -J, --json             use JSON output format\n"), out);
	fputs(_(" -l, --list             use list format output\n"), out);
	fputs(_(" -N, --task <tid>       use alternative namespace (/proc/<tid>/mountinfo file)\n"), out);
	fputs(_(" -n, --noheadings       don't print column headings\n"), out);
	fputs(_(" -O, --options <list>   limit the set of filesystems by mount options\n"), out);
	fputs(_(" -o, --output <list>    the output columns to be shown\n"), out);
	fputs(_("     --output-all       output all available columns\n"), out);
	fputs(_(" -P, --pairs            use key=\"value\" output format\n"), out);
	fputs(_("     --pseudo           print only pseudo-filesystems\n"), out);
	fputs(_("     --shadowed         print only filesystems over-mounted by another filesystem\n"), out);
	fputs(_(" -R, --submounts        print all submounts for the matching filesystems\n"), out);
	fputs(_(" -r, --raw              use raw output format\n"), out);
	fputs(_("     --real             print only real filesystems\n"), out);
	fputs(_(" -S, --source <string>  the device to mount (by name, maj:min, \n"
	        "                          LABEL=, UUID=, PARTUUID=, PARTLABEL=)\n"), out);
	fputs(_(" -T, --target <path>    the path to the filesystem to use\n"), out);
	fputs(_("     --tree             enable tree format output if possible\n"), out);
	fputs(_(" -M, --mountpoint <dir> the mountpoint directory\n"), out);
	fputs(_(" -t, --types <list>     limit the set of filesystems by FS types\n"), out);
	fputs(_(" -U, --uniq             ignore filesystems with duplicate target\n"), out);
	fputs(_(" -u, --notruncate       don't truncate text in columns\n"), out);
	fputs(_(" -v, --nofsroot         don't print [/dir] for bind or btrfs mounts\n"), out);

	fputc('\n', out);
	fputs(_(" -x, --verify           verify mount table content (default is fstab)\n"), out);
	fputs(_("     --verbose          print more details\n"), out);
	fputs(_("     --vfs-all          print all VFS options\n"), out);

	fputs(USAGE_SEPARATOR, out);
	printf(USAGE_HELP_OPTIONS(24));

	fputs(USAGE_COLUMNS, out);
	for (i = 0; i < ARRAY_SIZE(infos); i++)
		fprintf(out, " %11s  %s\n", infos[i].name, _(infos[i].help));

	printf(USAGE_MAN_TAIL("findmnt(8)"));

	exit(EXIT_SUCCESS);
}