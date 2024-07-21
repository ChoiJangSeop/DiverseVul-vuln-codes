static bool _db_chain_lt(const struct db_arg_chain_tree *a,
			 const struct db_arg_chain_tree *b)
{
	return ((a->arg < b->arg) ||
		((a->arg == b->arg) &&
		 ((a->op < b->op) ||
		  ((a->op == b->op) &&
		   ((a->mask < b->mask) ||
		    ((a->mask == b->mask) &&
		     (a->datum < b->datum)))))));
}