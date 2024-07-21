bucket_push (index_bucket *bucket,
             CK_OBJECT_HANDLE handle)
{
	unsigned int alloc;

	alloc = alloc_size (bucket->num);
	if (bucket->num + 1 > alloc) {
		CK_OBJECT_HANDLE *elem;

		alloc = alloc ? alloc * 2 : 1;
		return_val_if_fail (alloc != 0, false);
		elem = realloc (bucket->elem, alloc * sizeof (CK_OBJECT_HANDLE));
		return_val_if_fail (elem != NULL, false);
		bucket->elem = elem;
	}

	return_val_if_fail (bucket->elem != NULL, false);
	bucket->elem[bucket->num++] = handle;
	return true;
}