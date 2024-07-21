PgDatabase *add_database(const char *name)
{
	PgDatabase *db = find_database(name);

	/* create new object if needed */
	if (db == NULL) {
		db = slab_alloc(db_cache);
		if (!db)
			return NULL;

		list_init(&db->head);
		safe_strcpy(db->name, name, sizeof(db->name));
		put_in_order(&db->head, &database_list, cmp_database);
	}

	return db;
}