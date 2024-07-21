int read_file_stat_bunch(struct activity *act[], int curr, int ifd, int act_nr,
			 struct file_activity *file_actlst, int endian_mismatch,
			 int arch_64, char *dfile, struct file_magic *file_magic,
			 int oneof)
{
	int i, j, p;
	struct file_activity *fal = file_actlst;
	off_t offset;
	__nr_t nr_value;

	for (i = 0; i < act_nr; i++, fal++) {

		/* Read __nr_t value preceding statistics structures if it exists */
		if (fal->has_nr) {
			nr_value = read_nr_value(ifd, dfile, file_magic,
						 endian_mismatch, arch_64, FALSE);
		}
		else {
			nr_value = fal->nr;
		}

		if (nr_value > NR_MAX) {
#ifdef DEBUG
			fprintf(stderr, "%s: Value=%d Max=%d\n", __FUNCTION__, nr_value, NR_MAX);
#endif
			handle_invalid_sa_file(ifd, file_magic, dfile, 0);
		}

		if (((p = get_activity_position(act, fal->id, RESUME_IF_NOT_FOUND)) < 0) ||
		    (act[p]->magic != fal->magic)) {
			/*
			 * Ignore current activity in file, which is unknown to
			 * current sysstat version or has an unknown format.
			 */
			if (nr_value) {
				offset = (off_t) fal->size * (off_t) nr_value * (off_t) fal->nr2;
				if (lseek(ifd, offset, SEEK_CUR) < offset) {
					close(ifd);
					perror("lseek");
					if (oneof == UEOF_CONT)
						return 2;
					exit(2);
				}
			}
			continue;
		}

		if (nr_value > act[p]->nr_max) {
#ifdef DEBUG
			fprintf(stderr, "%s: %s: Value=%d Max=%d\n",
				__FUNCTION__, act[p]->name, nr_value, act[p]->nr_max);
#endif
			handle_invalid_sa_file(ifd, file_magic, dfile, 0);
		}
		act[p]->nr[curr] = nr_value;

		/* Reallocate buffers if needed */
		if (nr_value > act[p]->nr_allocated) {
			reallocate_all_buffers(act[p], nr_value);
		}

		/*
                 * For persistent activities, we must make sure that no statistics
                 * from a previous iteration remain, especially if the number
                 * of structures read is smaller than @nr_ini.
                 */
		if (HAS_PERSISTENT_VALUES(act[p]->options)) {
                    memset(act[p]->buf[curr], 0,
                           (size_t) act[p]->msize * (size_t) act[p]->nr_ini * (size_t) act[p]->nr2);
                }

		/* OK, this is a known activity: Read the stats structures */
		if ((nr_value > 0) &&
		    ((nr_value > 1) || (act[p]->nr2 > 1)) &&
		    (act[p]->msize > act[p]->fsize)) {

			for (j = 0; j < (nr_value * act[p]->nr2); j++) {
				if (sa_fread(ifd, (char *) act[p]->buf[curr] + j * act[p]->msize,
					 (size_t) act[p]->fsize, HARD_SIZE, oneof) > 0)
					/* Unexpected EOF */
					return 2;
			}
		}
		else if (nr_value > 0) {
			/*
			 * Note: If msize was smaller than fsize,
			 * then it has been set to fsize in check_file_actlst().
			 */
			if (sa_fread(ifd, act[p]->buf[curr],
				 (size_t) act[p]->fsize * (size_t) nr_value * (size_t) act[p]->nr2,
				 HARD_SIZE, oneof) > 0)
				/* Unexpected EOF */
				return 2;
		}
		else {
			/* nr_value == 0: Nothing to read */
			continue;
		}

		/* Normalize endianness for current activity's structures */
		if (endian_mismatch) {
			for (j = 0; j < (nr_value * act[p]->nr2); j++) {
				swap_struct(act[p]->ftypes_nr, (char *) act[p]->buf[curr] + j * act[p]->msize,
					    arch_64);
			}
		}

		/* Remap structure's fields to those known by current sysstat version */
		for (j = 0; j < (nr_value * act[p]->nr2); j++) {
			remap_struct(act[p]->gtypes_nr, act[p]->ftypes_nr,
				     (char *) act[p]->buf[curr] + j * act[p]->msize,
				     act[p]->fsize, act[p]->msize);
		}
	}

	return 0;
}