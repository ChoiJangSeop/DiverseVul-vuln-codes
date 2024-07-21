p11_rpc_message_alloc_extra_array (p11_rpc_message *msg,
				   size_t nmemb,
				   size_t size)
{
	if ((SIZE_MAX - sizeof (void *)) / nmemb < size) {
		errno = ENOMEM;
		return NULL;
	}
	return p11_rpc_message_alloc_extra (msg, nmemb * size);
}