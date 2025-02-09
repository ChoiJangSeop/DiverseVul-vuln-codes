void EC_GROUP_free(EC_GROUP *group)
	{
	if (!group) return;

	if (group->meth->group_finish != 0)
		group->meth->group_finish(group);

	EC_EX_DATA_free_all_data(&group->extra_data);

	if (group->generator != NULL)
		EC_POINT_free(group->generator);
	BN_free(&group->order);
	BN_free(&group->cofactor);

	if (group->seed)
		OPENSSL_free(group->seed);

	OPENSSL_free(group);
	}