static void _db_level_get(struct db_arg_chain_tree *node)
{
	struct db_arg_chain_tree *iter = node;

	while (iter->lvl_prv != NULL)
		iter = iter->lvl_prv;

	while (iter) {
		_db_tree_get(iter);
		iter = iter->lvl_nxt;
	}
}