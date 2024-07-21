_nc_find_type_entry(const char *string,
		    int type,
		    bool termcap)
{
    struct name_table_entry const *ptr = NULL;
    const HashData *data = _nc_get_hash_info(termcap);
    int hashvalue = data->hash_of(string);

    if (data->table_data[hashvalue] >= 0) {
	const struct name_table_entry *const table = _nc_get_table(termcap);

	ptr = table + data->table_data[hashvalue];
	while (ptr->nte_type != type
	       || !data->compare_names(ptr->nte_name, string)) {
	    if (ptr->nte_link < 0) {
		ptr = 0;
		break;
	    }
	    ptr = table + (ptr->nte_link + data->table_data[data->table_size]);
	}
    }

    return ptr;
}