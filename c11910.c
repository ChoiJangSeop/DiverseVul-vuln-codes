update_mtab (const char *dir, struct my_mntent *instead) {
	mntFILE *mfp, *mftmp;
	const char *fnam = _PATH_MOUNTED;
	struct mntentchn mtabhead;	/* dummy */
	struct mntentchn *mc, *mc0, *absent = NULL;
	struct stat sbuf;
	int fd;

	if (mtab_does_not_exist() || !mtab_is_writable())
		return;

	lock_mtab();

	/* having locked mtab, read it again */
	mc0 = mc = &mtabhead;
	mc->nxt = mc->prev = NULL;

	mfp = my_setmntent(fnam, "r");
	if (mfp == NULL || mfp->mntent_fp == NULL) {
		int errsv = errno;
		error (_("cannot open %s (%s) - mtab not updated"),
		       fnam, strerror (errsv));
		goto leave;
	}

	read_mntentchn(mfp, fnam, mc);

	/* find last occurrence of dir */
	for (mc = mc0->prev; mc && mc != mc0; mc = mc->prev)
		if (streq(mc->m.mnt_dir, dir))
			break;
	if (mc && mc != mc0) {
		if (instead == NULL) {
			/* An umount - remove entry */
			if (mc && mc != mc0) {
				mc->prev->nxt = mc->nxt;
				mc->nxt->prev = mc->prev;
				my_free_mc(mc);
			}
		} else if (!strcmp(mc->m.mnt_dir, instead->mnt_dir)) {
			/* A remount */
			char *opts = mk_remount_opts(mc->m.mnt_opts,
					instead->mnt_opts);
			my_free(mc->m.mnt_opts);
			mc->m.mnt_opts = opts;
		} else {
			/* A move */
			my_free(mc->m.mnt_dir);
			mc->m.mnt_dir = xstrdup(instead->mnt_dir);
		}
	} else if (instead) {
		/* not found, add a new entry */
		absent = xmalloc(sizeof(*absent));
		absent->m.mnt_fsname = xstrdup(instead->mnt_fsname);
		absent->m.mnt_dir = xstrdup(instead->mnt_dir);
		absent->m.mnt_type = xstrdup(instead->mnt_type);
		absent->m.mnt_opts = xstrdup(instead->mnt_opts);
		absent->m.mnt_freq = instead->mnt_freq;
		absent->m.mnt_passno = instead->mnt_passno;
		absent->nxt = mc0;
		if (mc0->prev != NULL) {
			absent->prev = mc0->prev;
			mc0->prev->nxt = absent;
		} else {
			absent->prev = mc0;
		}
		mc0->prev = absent;
		if (mc0->nxt == NULL)
			mc0->nxt = absent;
	}

	/* write chain to mtemp */
	mftmp = my_setmntent (_PATH_MOUNTED_TMP, "w");
	if (mftmp == NULL || mftmp->mntent_fp == NULL) {
		int errsv = errno;
		error (_("cannot open %s (%s) - mtab not updated"),
		       _PATH_MOUNTED_TMP, strerror (errsv));
		discard_mntentchn(mc0);
		goto leave;
	}

	for (mc = mc0->nxt; mc && mc != mc0; mc = mc->nxt) {
		if (my_addmntent(mftmp, &(mc->m)) == 1) {
			int errsv = errno;
			die (EX_FILEIO, _("error writing %s: %s"),
			     _PATH_MOUNTED_TMP, strerror (errsv));
		}
	}

	discard_mntentchn(mc0);
	fd = fileno(mftmp->mntent_fp);

	/*
	 * It seems that better is incomplete and broken /mnt/mtab that
	 * /mnt/mtab that is writeable for non-root users.
	 *
	 * We always skip rename() when chown() and chmod() failed.
	 * -- kzak, 11-Oct-2007
	 */

	if (fchmod(fd, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) < 0) {
		int errsv = errno;
		fprintf(stderr, _("error changing mode of %s: %s\n"),
			_PATH_MOUNTED_TMP, strerror (errsv));
		goto leave;
	}

	/*
	 * If mount is setuid and some non-root user mounts sth,
	 * then mtab.tmp might get the group of this user. Copy uid/gid
	 * from the present mtab before renaming.
	 */
	if (stat(_PATH_MOUNTED, &sbuf) == 0) {
		if (fchown(fd, sbuf.st_uid, sbuf.st_gid) < 0) {
			int errsv = errno;
			fprintf (stderr, _("error changing owner of %s: %s\n"),
				_PATH_MOUNTED_TMP, strerror(errsv));
			goto leave;
		}
	}

	my_endmntent (mftmp);

	/* rename mtemp to mtab */
	if (rename (_PATH_MOUNTED_TMP, _PATH_MOUNTED) < 0) {
		int errsv = errno;
		fprintf(stderr, _("can't rename %s to %s: %s\n"),
			_PATH_MOUNTED_TMP, _PATH_MOUNTED, strerror(errsv));
	}

 leave:
	unlock_mtab();
}