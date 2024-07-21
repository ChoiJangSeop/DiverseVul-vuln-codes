jpc_tagtree_t *jpc_tagtree_create(int numleafsh, int numleafsv)
{
	int nplh[JPC_TAGTREE_MAXDEPTH];
	int nplv[JPC_TAGTREE_MAXDEPTH];
	jpc_tagtreenode_t *node;
	jpc_tagtreenode_t *parentnode;
	jpc_tagtreenode_t *parentnode0;
	jpc_tagtree_t *tree;
	int i;
	int j;
	int k;
	int numlvls;
	int n;

	assert(numleafsh > 0 && numleafsv > 0);

	if (!(tree = jpc_tagtree_alloc())) {
		return 0;
	}
	tree->numleafsh_ = numleafsh;
	tree->numleafsv_ = numleafsv;

	numlvls = 0;
	nplh[0] = numleafsh;
	nplv[0] = numleafsv;
	do {
		n = nplh[numlvls] * nplv[numlvls];
		nplh[numlvls + 1] = (nplh[numlvls] + 1) / 2;
		nplv[numlvls + 1] = (nplv[numlvls] + 1) / 2;
		tree->numnodes_ += n;
		++numlvls;
	} while (n > 1);

	if (!(tree->nodes_ = jas_malloc(tree->numnodes_ * sizeof(jpc_tagtreenode_t)))) {
		return 0;
	}

	/* Initialize the parent links for all nodes in the tree. */

	node = tree->nodes_;
	parentnode = &tree->nodes_[tree->numleafsh_ * tree->numleafsv_];
	parentnode0 = parentnode;

	for (i = 0; i < numlvls - 1; ++i) {
		for (j = 0; j < nplv[i]; ++j) {
			k = nplh[i];
			while (--k >= 0) {
				node->parent_ = parentnode;
				++node;
				if (--k >= 0) {
					node->parent_ = parentnode;
					++node;
				}
				++parentnode;
			}
			if ((j & 1) || j == nplv[i] - 1) {
				parentnode0 = parentnode;
			} else {
				parentnode = parentnode0;
				parentnode0 += nplh[i];
			}
		}
	}
	node->parent_ = 0;

	/* Initialize the data values to something sane. */

	jpc_tagtree_reset(tree);

	return tree;
}