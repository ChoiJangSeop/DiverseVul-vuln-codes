authentic_set_current_files(struct sc_card *card, struct sc_path *path,
		unsigned char *resp, size_t resplen, struct sc_file **file_out)
{
	struct sc_context *ctx = card->ctx;
	struct sc_file *file = NULL;
	int rv;

	LOG_FUNC_CALLED(ctx);
	if (resplen)   {
		switch (resp[0]) {
		case 0x62:
		case 0x6F:
			file = sc_file_new();
			if (file == NULL)
				LOG_FUNC_RETURN(ctx, SC_ERROR_OUT_OF_MEMORY);
			if (path)
				file->path = *path;

			rv = authentic_process_fci(card, file, resp, resplen);
			LOG_TEST_RET(ctx, rv, "cannot set 'current file': FCI process error");

			break;
		default:
			LOG_FUNC_RETURN(ctx, SC_ERROR_UNKNOWN_DATA_RECEIVED);
		}

		if (file->type == SC_FILE_TYPE_DF)   {
			struct sc_path cur_df_path;

			memset(&cur_df_path, 0, sizeof(cur_df_path));
			if (card->cache.valid && card->cache.current_df)   {
				cur_df_path = card->cache.current_df->path;
				sc_file_free(card->cache.current_df);
			}
			card->cache.current_df = NULL;
			sc_file_dup(&card->cache.current_df, file);

			if (cur_df_path.len)   {
				memcpy(card->cache.current_df->path.value + cur_df_path.len,
						card->cache.current_df->path.value,
						card->cache.current_df->path.len);
				memcpy(card->cache.current_df->path.value, cur_df_path.value, cur_df_path.len);
				card->cache.current_df->path.len += cur_df_path.len;
			}

			sc_file_free(card->cache.current_ef);
			card->cache.current_ef = NULL;

			card->cache.valid = 1;
		}
		else   {
			sc_file_free(card->cache.current_ef);
			card->cache.current_ef = NULL;
			sc_file_dup(&card->cache.current_ef, file);
		}

		if (file_out)
			*file_out = file;
		else
			sc_file_free(file);
	}

	LOG_FUNC_RETURN(ctx, SC_SUCCESS);
}