bucket_insert (index_bucket *bucket,
               CK_OBJECT_HANDLE handle)
{
	unsigned int alloc;
	int at = 0;

	if (bucket->elem) {
		at = binary_search (bucket->elem, 0, bucket->num, handle);
		if (at < bucket->num && bucket->elem[at] == handle)
			return;
	}

	alloc = alloc_size (bucket->num);
	if (bucket->num + 1 > alloc) {
		CK_OBJECT_HANDLE *elem;

		alloc = alloc ? alloc * 2 : 1;
		return_if_fail (alloc != 0);
		elem = realloc (bucket->elem, alloc * sizeof (CK_OBJECT_HANDLE));
		return_if_fail (elem != NULL);
		bucket->elem = elem;
	}

	return_if_fail (bucket->elem != NULL);
	memmove (bucket->elem + at + 1, bucket->elem + at,
	         (bucket->num - at) * sizeof (CK_OBJECT_HANDLE));
	bucket->elem[at] = handle;
	bucket->num++;
}