static int _db_tree_add(struct db_arg_chain_tree **existing,
			struct db_arg_chain_tree *new,
			struct db_iter_state *state)
{
	int rc;
	struct db_arg_chain_tree *x_iter = *existing;
	struct db_arg_chain_tree *n_iter = new;

	/* TODO: when we add sub-trees to the current level (see the lt/gt
	 * branches below) we need to grab an extra reference for sub-trees at
	 * the current as the current level is visible to both the new and
	 * existing trees; at some point if would be nice if we didn't have to
	 * take this extra reference */

	do {
		if (_db_chain_eq(x_iter, n_iter)) {
			if (n_iter->act_t_flg) {
				if (!x_iter->act_t_flg) {
					/* new node has a true action */

					/* do the actions match? */
					rc = _db_tree_act_check(x_iter->nxt_t,
								n_iter->act_t);
					if (rc != 0)
						return rc;

					/* update with the new action */
					rc = _db_tree_put(&x_iter->nxt_t);
					x_iter->nxt_t = NULL;
					x_iter->act_t = n_iter->act_t;
					x_iter->act_t_flg = true;
					state->sx->node_cnt -= rc;
				} else if (n_iter->act_t != x_iter->act_t)
					return -EEXIST;
			}
			if (n_iter->act_f_flg) {
				if (!x_iter->act_f_flg) {
					/* new node has a false action */

					/* do the actions match? */
					rc = _db_tree_act_check(x_iter->nxt_f,
								n_iter->act_f);
					if (rc != 0)
						return rc;

					/* update with the new action */
					rc = _db_tree_put(&x_iter->nxt_f);
					x_iter->nxt_f = NULL;
					x_iter->act_f = n_iter->act_f;
					x_iter->act_f_flg = true;
					state->sx->node_cnt -= rc;
				} else if (n_iter->act_f != x_iter->act_f)
					return -EEXIST;
			}

			if (n_iter->nxt_t) {
				if (x_iter->nxt_t) {
					/* compare the next level */
					rc = _db_tree_add(&x_iter->nxt_t,
							  n_iter->nxt_t,
							  state);
					if (rc != 0)
						return rc;
				} else if (!x_iter->act_t_flg) {
					/* add a new sub-tree */
					x_iter->nxt_t = _db_tree_get(n_iter->nxt_t);
				} else
					/* done - existing tree is "shorter" */
					return 0;
			}
			if (n_iter->nxt_f) {
				if (x_iter->nxt_f) {
					/* compare the next level */
					rc = _db_tree_add(&x_iter->nxt_f,
							  n_iter->nxt_f,
							  state);
					if (rc != 0)
						return rc;
				} else if (!x_iter->act_f_flg) {
					/* add a new sub-tree */
					x_iter->nxt_f = _db_tree_get(n_iter->nxt_f);
				} else
					/* done - existing tree is "shorter" */
					return 0;
			}

			return 0;
		} else if (!_db_chain_lt(x_iter, n_iter)) {
			/* try to move along the current level */
			if (x_iter->lvl_nxt == NULL) {
				/* add to the end of this level */
				_db_level_get(x_iter);
				_db_tree_get(n_iter);
				n_iter->lvl_prv = x_iter;
				x_iter->lvl_nxt = n_iter;
				return 0;
			} else
				/* next */
				x_iter = x_iter->lvl_nxt;
		} else {
			/* add before the existing node on this level*/
			_db_level_get(x_iter);
			_db_tree_get(n_iter);
			if (x_iter->lvl_prv != NULL)
				x_iter->lvl_prv->lvl_nxt = n_iter;
			n_iter->lvl_prv = x_iter->lvl_prv;
			n_iter->lvl_nxt = x_iter;
			x_iter->lvl_prv = n_iter;
			if (*existing == x_iter)
				*existing = n_iter;
			return 0;
		}
	} while (x_iter);

	return 0;
}