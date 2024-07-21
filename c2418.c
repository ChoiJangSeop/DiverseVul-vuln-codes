pq_discardbytes(size_t len)
{
	size_t		amount;

	while (len > 0)
	{
		while (PqRecvPointer >= PqRecvLength)
		{
			if (pq_recvbuf())	/* If nothing in buffer, then recv some */
				return EOF;		/* Failed to recv data */
		}
		amount = PqRecvLength - PqRecvPointer;
		if (amount > len)
			amount = len;
		PqRecvPointer += amount;
		len -= amount;
	}
	return 0;
}