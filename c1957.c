int apparmor_bprm_set_creds(struct linux_binprm *bprm)
{
	struct aa_task_cxt *cxt;
	struct aa_profile *profile, *new_profile = NULL;
	struct aa_namespace *ns;
	char *buffer = NULL;
	unsigned int state;
	struct file_perms perms = {};
	struct path_cond cond = {
		bprm->file->f_path.dentry->d_inode->i_uid,
		bprm->file->f_path.dentry->d_inode->i_mode
	};
	const char *name = NULL, *target = NULL, *info = NULL;
	int error = cap_bprm_set_creds(bprm);
	if (error)
		return error;

	if (bprm->cred_prepared)
		return 0;

	cxt = bprm->cred->security;
	BUG_ON(!cxt);

	profile = aa_get_profile(aa_newest_version(cxt->profile));
	/*
	 * get the namespace from the replacement profile as replacement
	 * can change the namespace
	 */
	ns = profile->ns;
	state = profile->file.start;

	/* buffer freed below, name is pointer into buffer */
	error = aa_path_name(&bprm->file->f_path, profile->path_flags, &buffer,
			     &name, &info);
	if (error) {
		if (profile->flags &
		    (PFLAG_IX_ON_NAME_ERROR | PFLAG_UNCONFINED))
			error = 0;
		name = bprm->filename;
		goto audit;
	}

	/* Test for onexec first as onexec directives override other
	 * x transitions.
	 */
	if (unconfined(profile)) {
		/* unconfined task */
		if (cxt->onexec)
			/* change_profile on exec already been granted */
			new_profile = aa_get_profile(cxt->onexec);
		else
			new_profile = find_attach(ns, &ns->base.profiles, name);
		if (!new_profile)
			goto cleanup;
		goto apply;
	}

	/* find exec permissions for name */
	state = aa_str_perms(profile->file.dfa, state, name, &cond, &perms);
	if (cxt->onexec) {
		struct file_perms cp;
		info = "change_profile onexec";
		if (!(perms.allow & AA_MAY_ONEXEC))
			goto audit;

		/* test if this exec can be paired with change_profile onexec.
		 * onexec permission is linked to exec with a standard pairing
		 * exec\0change_profile
		 */
		state = aa_dfa_null_transition(profile->file.dfa, state);
		cp = change_profile_perms(profile, cxt->onexec->ns,
					  cxt->onexec->base.name,
					  AA_MAY_ONEXEC, state);

		if (!(cp.allow & AA_MAY_ONEXEC))
			goto audit;
		new_profile = aa_get_profile(aa_newest_version(cxt->onexec));
		goto apply;
	}

	if (perms.allow & MAY_EXEC) {
		/* exec permission determine how to transition */
		new_profile = x_to_profile(profile, name, perms.xindex);
		if (!new_profile) {
			if (perms.xindex & AA_X_INHERIT) {
				/* (p|c|n)ix - don't change profile but do
				 * use the newest version, which was picked
				 * up above when getting profile
				 */
				info = "ix fallback";
				new_profile = aa_get_profile(profile);
				goto x_clear;
			} else if (perms.xindex & AA_X_UNCONFINED) {
				new_profile = aa_get_profile(ns->unconfined);
				info = "ux fallback";
			} else {
				error = -ENOENT;
				info = "profile not found";
			}
		}
	} else if (COMPLAIN_MODE(profile)) {
		/* no exec permission - are we in learning mode */
		new_profile = aa_new_null_profile(profile, 0);
		if (!new_profile) {
			error = -ENOMEM;
			info = "could not create null profile";
		} else {
			error = -EACCES;
			target = new_profile->base.hname;
		}
		perms.xindex |= AA_X_UNSAFE;
	} else
		/* fail exec */
		error = -EACCES;

	if (!new_profile)
		goto audit;

	if (bprm->unsafe & LSM_UNSAFE_SHARE) {
		/* FIXME: currently don't mediate shared state */
		;
	}

	if (bprm->unsafe & (LSM_UNSAFE_PTRACE | LSM_UNSAFE_PTRACE_CAP)) {
		error = may_change_ptraced_domain(current, new_profile);
		if (error) {
			aa_put_profile(new_profile);
			goto audit;
		}
	}

	/* Determine if secure exec is needed.
	 * Can be at this point for the following reasons:
	 * 1. unconfined switching to confined
	 * 2. confined switching to different confinement
	 * 3. confined switching to unconfined
	 *
	 * Cases 2 and 3 are marked as requiring secure exec
	 * (unless policy specified "unsafe exec")
	 *
	 * bprm->unsafe is used to cache the AA_X_UNSAFE permission
	 * to avoid having to recompute in secureexec
	 */
	if (!(perms.xindex & AA_X_UNSAFE)) {
		AA_DEBUG("scrubbing environment variables for %s profile=%s\n",
			 name, new_profile->base.hname);
		bprm->unsafe |= AA_SECURE_X_NEEDED;
	}
apply:
	target = new_profile->base.hname;
	/* when transitioning profiles clear unsafe personality bits */
	bprm->per_clear |= PER_CLEAR_ON_SETID;

x_clear:
	aa_put_profile(cxt->profile);
	/* transfer new profile reference will be released when cxt is freed */
	cxt->profile = new_profile;

	/* clear out all temporary/transitional state from the context */
	aa_put_profile(cxt->previous);
	aa_put_profile(cxt->onexec);
	cxt->previous = NULL;
	cxt->onexec = NULL;
	cxt->token = 0;

audit:
	error = aa_audit_file(profile, &perms, GFP_KERNEL, OP_EXEC, MAY_EXEC,
			      name, target, cond.uid, info, error);

cleanup:
	aa_put_profile(profile);
	kfree(buffer);

	return error;
}