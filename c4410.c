void jas_free(void *ptr)
{
	JAS_DBGLOG(100, ("jas_free(%p)\n", ptr));
	free(ptr);
}