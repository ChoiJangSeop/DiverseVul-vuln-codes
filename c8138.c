static git_commit_list_node **alloc_parents(
	git_revwalk *walk, git_commit_list_node *commit, size_t n_parents)
{
	if (n_parents <= PARENTS_PER_COMMIT)
		return (git_commit_list_node **)((char *)commit + sizeof(git_commit_list_node));

	return (git_commit_list_node **)git_pool_malloc(
		&walk->commit_pool, (uint32_t)(n_parents * sizeof(git_commit_list_node *)));
}