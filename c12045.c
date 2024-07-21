static int count(struct cstate *g, Renode *node)
{
	int min, max, n;
	if (!node) return 0;
	switch (node->type) {
	default: return 1;
	case P_CAT: return count(g, node->x) + count(g, node->y);
	case P_ALT: return count(g, node->x) + count(g, node->y) + 2;
	case P_REP:
		min = node->m;
		max = node->n;
		if (min == max) n = count(g, node->x) * min;
		else if (max < REPINF) n = count(g, node->x) * max + (max - min);
		else n = count(g, node->x) * (min + 1) + 2;
		if (n < 0 || n > REG_MAXPROG) die(g, "program too large");
		return n;
	case P_PAR: return count(g, node->x) + 2;
	case P_PLA: return count(g, node->x) + 2;
	case P_NLA: return count(g, node->x) + 2;
	}
}