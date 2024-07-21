set_fflags(struct archive_write_disk *a)
{
	struct fixup_entry *le;
	unsigned long	set, clear;
	int		r;
	mode_t		mode = archive_entry_mode(a->entry);
	/*
	 * Make 'critical_flags' hold all file flags that can't be
	 * immediately restored.  For example, on BSD systems,
	 * SF_IMMUTABLE prevents hardlinks from being created, so
	 * should not be set until after any hardlinks are created.  To
	 * preserve some semblance of portability, this uses #ifdef
	 * extensively.  Ugly, but it works.
	 *
	 * Yes, Virginia, this does create a security race.  It's mitigated
	 * somewhat by the practice of creating dirs 0700 until the extract
	 * is done, but it would be nice if we could do more than that.
	 * People restoring critical file systems should be wary of
	 * other programs that might try to muck with files as they're
	 * being restored.
	 */
	const int	critical_flags = 0
#ifdef SF_IMMUTABLE
	    | SF_IMMUTABLE
#endif
#ifdef UF_IMMUTABLE
	    | UF_IMMUTABLE
#endif
#ifdef SF_APPEND
	    | SF_APPEND
#endif
#ifdef UF_APPEND
	    | UF_APPEND
#endif
#if defined(FS_APPEND_FL)
	    | FS_APPEND_FL
#elif defined(EXT2_APPEND_FL)
	    | EXT2_APPEND_FL
#endif
#if defined(FS_IMMUTABLE_FL)
	    | FS_IMMUTABLE_FL
#elif defined(EXT2_IMMUTABLE_FL)
	    | EXT2_IMMUTABLE_FL
#endif
#ifdef FS_JOURNAL_DATA_FL
	    | FS_JOURNAL_DATA_FL
#endif
	;

	if (a->todo & TODO_FFLAGS) {
		archive_entry_fflags(a->entry, &set, &clear);

		/*
		 * The first test encourages the compiler to eliminate
		 * all of this if it's not necessary.
		 */
		if ((critical_flags != 0)  &&  (set & critical_flags)) {
			le = current_fixup(a, a->name);
			if (le == NULL)
				return (ARCHIVE_FATAL);
			le->fixup |= TODO_FFLAGS;
			le->fflags_set = set;
			/* Store the mode if it's not already there. */
			if ((le->fixup & TODO_MODE) == 0)
				le->mode = mode;
		} else {
			r = set_fflags_platform(a, a->fd,
			    a->name, mode, set, clear);
			if (r != ARCHIVE_OK)
				return (r);
		}
	}
	return (ARCHIVE_OK);
}