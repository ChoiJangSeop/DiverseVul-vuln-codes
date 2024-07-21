int genl_register_family(struct genl_family *family)
{
	int err, i;
	int start = GENL_START_ALLOC, end = GENL_MAX_ID;

	err = genl_validate_ops(family);
	if (err)
		return err;

	genl_lock_all();

	if (genl_family_find_byname(family->name)) {
		err = -EEXIST;
		goto errout_locked;
	}

	/*
	 * Sadly, a few cases need to be special-cased
	 * due to them having previously abused the API
	 * and having used their family ID also as their
	 * multicast group ID, so we use reserved IDs
	 * for both to be sure we can do that mapping.
	 */
	if (family == &genl_ctrl) {
		/* and this needs to be special for initial family lookups */
		start = end = GENL_ID_CTRL;
	} else if (strcmp(family->name, "pmcraid") == 0) {
		start = end = GENL_ID_PMCRAID;
	} else if (strcmp(family->name, "VFS_DQUOT") == 0) {
		start = end = GENL_ID_VFS_DQUOT;
	}

	if (family->maxattr && !family->parallel_ops) {
		family->attrbuf = kmalloc_array(family->maxattr + 1,
						sizeof(struct nlattr *),
						GFP_KERNEL);
		if (family->attrbuf == NULL) {
			err = -ENOMEM;
			goto errout_locked;
		}
	} else
		family->attrbuf = NULL;

	family->id = idr_alloc(&genl_fam_idr, family,
			       start, end + 1, GFP_KERNEL);
	if (family->id < 0) {
		err = family->id;
		goto errout_locked;
	}

	err = genl_validate_assign_mc_groups(family);
	if (err)
		goto errout_remove;

	genl_unlock_all();

	/* send all events */
	genl_ctrl_event(CTRL_CMD_NEWFAMILY, family, NULL, 0);
	for (i = 0; i < family->n_mcgrps; i++)
		genl_ctrl_event(CTRL_CMD_NEWMCAST_GRP, family,
				&family->mcgrps[i], family->mcgrp_offset + i);

	return 0;

errout_remove:
	idr_remove(&genl_fam_idr, family->id);
	kfree(family->attrbuf);
errout_locked:
	genl_unlock_all();
	return err;
}