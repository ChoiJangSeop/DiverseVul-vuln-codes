prepend_row_security_policies(Query* root, RangeTblEntry* rte, int rt_index)
{
	Expr			   *rowsec_expr = NULL;
	Expr			   *rowsec_with_check_expr = NULL;
	Expr			   *hook_expr = NULL;
	Expr			   *hook_with_check_expr = NULL;

	List			   *rowsec_policies;
	List			   *hook_policies = NIL;

	Relation 			rel;
	Oid					user_id;
	int					sec_context;
	int					rls_status;
	bool				defaultDeny = true;
	bool				hassublinks = false;

	/* This is just to get the security context */
	GetUserIdAndSecContext(&user_id, &sec_context);

	/* Switch to checkAsUser if it's set */
	user_id = rte->checkAsUser ? rte->checkAsUser : GetUserId();

	/*
	 * If this is not a normal relation, or we have been told
	 * to explicitly skip RLS (perhaps because this is an FK check)
	 * then just return immediately.
	 */
	if (rte->relid < FirstNormalObjectId
		|| rte->relkind != RELKIND_RELATION
		|| (sec_context & SECURITY_ROW_LEVEL_DISABLED))
		return false;

	/* Determine the state of RLS for this, pass checkAsUser explicitly */
	rls_status = check_enable_rls(rte->relid, rte->checkAsUser);

	/* If there is no RLS on this table at all, nothing to do */
	if (rls_status == RLS_NONE)
		return false;

	/*
	 * RLS_NONE_ENV means we are not doing any RLS now, but that may change
	 * with changes to the environment, so we mark it as hasRowSecurity to
	 * force a re-plan when the environment changes.
	 */
	if (rls_status == RLS_NONE_ENV)
	{
		/*
		 * Indicate that this query may involve RLS and must therefore
		 * be replanned if the environment changes (GUCs, role), but we
		 * are not adding anything here.
		 */
		root->hasRowSecurity = true;

		return false;
	}

	/*
	 * We may end up getting called multiple times for the same RTE, so check
	 * to make sure we aren't doing double-work.
	 */
	if (rte->securityQuals != NIL)
		return false;

	/* Grab the built-in policies which should be applied to this relation. */
	rel = heap_open(rte->relid, NoLock);

	rowsec_policies = pull_row_security_policies(root->commandType, rel,
												 user_id);

	/*
	 * Check if this is only the default-deny policy.
	 *
	 * Normally, if the table has row security enabled but there are
	 * no policies, we use a default-deny policy and not allow anything.
	 * However, when an extension uses the hook to add their own
	 * policies, we don't want to include the default deny policy or
	 * there won't be any way for a user to use an extension exclusively
	 * for the policies to be used.
	 */
	if (((RowSecurityPolicy *) linitial(rowsec_policies))->policy_id
			== InvalidOid)
		defaultDeny = true;

	/* Now that we have our policies, build the expressions from them. */
	process_policies(rowsec_policies, rt_index, &rowsec_expr,
					 &rowsec_with_check_expr, &hassublinks);

	/*
	 * Also, allow extensions to add their own policies.
	 *
	 * Note that, as with the internal policies, if multiple policies are
	 * returned then they will be combined into a single expression with
	 * all of them OR'd together.  However, to avoid the situation of an
	 * extension granting more access to a table than the internal policies
	 * would allow, the extension's policies are AND'd with the internal
	 * policies.  In other words - extensions can only provide further
	 * filtering of the result set (or further reduce the set of records
	 * allowed to be added).
	 *
	 * If only a USING policy is returned by the extension then it will be
	 * used for WITH CHECK as well, similar to how internal policies are
	 * handled.
	 *
	 * The only caveat to this is that if there are NO internal policies
	 * defined, there ARE policies returned by the extension, and RLS is
	 * enabled on the table, then we will ignore the internally-generated
	 * default-deny policy and use only the policies returned by the
	 * extension.
	 */
	if (row_security_policy_hook)
	{
		hook_policies = (*row_security_policy_hook)(root->commandType, rel);

		/* Build the expression from any policies returned. */
		process_policies(hook_policies, rt_index, &hook_expr,
						 &hook_with_check_expr, &hassublinks);
	}

	/*
	 * If the only built-in policy is the default-deny one, and hook
	 * policies exist, then use the hook policies only and do not apply
	 * the default-deny policy.  Otherwise, apply both sets (AND'd
	 * together).
	 */
	if (defaultDeny && hook_policies != NIL)
		rowsec_expr = NULL;

	/*
	 * For INSERT or UPDATE, we need to add the WITH CHECK quals to
	 * Query's withCheckOptions to verify that any new records pass the
	 * WITH CHECK policy (this will be a copy of the USING policy, if no
	 * explicit WITH CHECK policy exists).
	 */
	if (root->commandType == CMD_INSERT || root->commandType == CMD_UPDATE)
	{
		/*
		 * WITH CHECK OPTIONS wants a WCO node which wraps each Expr, so
		 * create them as necessary.
		 */
		if (rowsec_with_check_expr)
		{
			WithCheckOption	   *wco;

			wco = (WithCheckOption *) makeNode(WithCheckOption);
			wco->viewname = RelationGetRelationName(rel);
			wco->qual = (Node *) rowsec_with_check_expr;
			wco->cascaded = false;
			root->withCheckOptions = lcons(wco, root->withCheckOptions);
		}

		/*
		 * Ditto for the expression, if any, returned from the extension.
		 */
		if (hook_with_check_expr)
		{
			WithCheckOption	   *wco;

			wco = (WithCheckOption *) makeNode(WithCheckOption);
			wco->viewname = RelationGetRelationName(rel);
			wco->qual = (Node *) hook_with_check_expr;
			wco->cascaded = false;
			root->withCheckOptions = lcons(wco, root->withCheckOptions);
		}
	}

	/* For SELECT, UPDATE, and DELETE, set the security quals */
	if (root->commandType == CMD_SELECT
		|| root->commandType == CMD_UPDATE
		|| root->commandType == CMD_DELETE)
	{
		if (rowsec_expr)
			rte->securityQuals = lcons(rowsec_expr, rte->securityQuals);

		if (hook_expr)
			rte->securityQuals = lcons(hook_expr,
									   rte->securityQuals);
	}

	heap_close(rel, NoLock);

	/*
	 * Mark this query as having row security, so plancache can invalidate
	 * it when necessary (eg: role changes)
	 */
	root->hasRowSecurity = true;

	/*
	 * If we have sublinks added because of the policies being added to the
	 * query, then set hasSubLinks on the Query to force subLinks to be
	 * properly expanded.
	 */
	root->hasSubLinks |= hassublinks;

	/* If we got this far, we must have added quals */
	return true;
}