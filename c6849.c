static HashTable *com_properties_get(zval *object)
{
	/* TODO: use type-info to get all the names and values ?
	 * DANGER: if we do that, there is a strong possibility for
	 * infinite recursion when the hash is displayed via var_dump().
	 * Perhaps it is best to leave it un-implemented.
	 */
	return NULL;
}