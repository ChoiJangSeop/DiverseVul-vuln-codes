_nc_find_user_entry(const char *string)
{
    const HashData *data = _nc_get_hash_user();
    int hashvalue;
    struct user_table_entry const *ptr = 0;
    struct user_table_entry const *real_table;

    hashvalue = data->hash_of(string);

    if (data->table_data[hashvalue] >= 0) {

	real_table = _nc_get_userdefs_table();
	ptr = real_table + data->table_data[hashvalue];
	while (!data->compare_names(ptr->ute_name, string)) {
	    if (ptr->ute_link < 0) {
		ptr = 0;
		break;
	    }
	    ptr = real_table + (ptr->ute_link
				+ data->table_data[data->table_size]);
	}
    }

    return (ptr);
}