void EC_GROUP_clear_free(EC_GROUP *group)
	{
	if (!group) return;

	if (group->meth->group_clear_finish != 0)
		group->meth->group_clear_finish(group);
	else if (group->meth->group_finish != 0)
		group->meth->group_finish(group);

	EC_EX_DATA_clear_free_all_data(&group->extra_data);

	if (group->generator != NULL)
		EC_POINT_clear_free(group->generator);
	BN_clear_free(&group->order);
	BN_clear_free(&group->cofactor);

	if (group->seed)
		{
		OPENSSL_cleanse(group->seed, group->seed_len);
		OPENSSL_free(group->seed);
		}

	OPENSSL_cleanse(group, sizeof *group);
	OPENSSL_free(group);
	}