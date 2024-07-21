pq_getbyte_if_available(unsigned char *c)
{
	int			r;

	if (PqRecvPointer < PqRecvLength)
	{
		*c = PqRecvBuffer[PqRecvPointer++];
		return 1;
	}

	/* Put the socket into non-blocking mode */
	socket_set_nonblocking(true);

	r = secure_read(MyProcPort, c, 1);
	if (r < 0)
	{
		/*
		 * Ok if no data available without blocking or interrupted (though
		 * EINTR really shouldn't happen with a non-blocking socket). Report
		 * other errors.
		 */
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			r = 0;
		else
		{
			/*
			 * Careful: an ereport() that tries to write to the client would
			 * cause recursion to here, leading to stack overflow and core
			 * dump!  This message must go *only* to the postmaster log.
			 */
			ereport(COMMERROR,
					(errcode_for_socket_access(),
					 errmsg("could not receive data from client: %m")));
			r = EOF;
		}
	}
	else if (r == 0)
	{
		/* EOF detected */
		r = EOF;
	}

	return r;
}