pq_getbyte(void)
{
	while (PqRecvPointer >= PqRecvLength)
	{
		if (pq_recvbuf())		/* If nothing in buffer, then recv some */
			return EOF;			/* Failed to recv data */
	}
	return (unsigned char) PqRecvBuffer[PqRecvPointer++];
}