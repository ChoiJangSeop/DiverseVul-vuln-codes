static bool _db_chain_eq(const struct db_arg_chain_tree *a,
			 const struct db_arg_chain_tree *b)
{
	return ((a->arg == b->arg) && (a->op == b->op) &&
		(a->datum == b->datum) && (a->mask == b->mask));
}