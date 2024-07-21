static void save_key_to(const char *algo, const char *name, const char *keydata)
{
	const char *error;
	struct dict_transaction_context *ctx =
		dict_transaction_begin(keys_dict);

	dict_set(ctx, t_strconcat(DICT_PATH_SHARED, "default/", algo, "/",
				  name, NULL),
		 keydata);
	if (dict_transaction_commit(&ctx, &error) < 0)
		i_error("dict_set(%s) failed: %s", name, error);
}