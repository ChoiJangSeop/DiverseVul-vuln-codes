static void try_free_empty_list(RzList *list) {
	if (list != NULL) {
		rz_list_free(list);
	}
}