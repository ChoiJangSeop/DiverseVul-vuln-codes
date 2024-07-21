_nc_find_entry(const char *string,
	       const HashValue * hash_table)
{
    bool termcap = (hash_table != _nc_get_hash_table(FALSE));
    const HashData *data = _nc_get_hash_info(termcap);
    int hashvalue;
    struct name_table_entry const *ptr = 0;
    struct name_table_entry const *real_table;

    hashvalue = data->hash_of(string);

    if (data->table_data[hashvalue] >= 0) {

	real_table = _nc_get_table(termcap);
	ptr = real_table + data->table_data[hashvalue];
	while (!data->compare_names(ptr->nte_name, string)) {
	    if (ptr->nte_link < 0) {
		ptr = 0;
		break;
	    }
	    ptr = real_table + (ptr->nte_link
				+ data->table_data[data->table_size]);
	}
    }

    return (ptr);
}