pq_init(void)
{
	PqSendBufferSize = PQ_SEND_BUFFER_SIZE;
	PqSendBuffer = MemoryContextAlloc(TopMemoryContext, PqSendBufferSize);
	PqSendPointer = PqSendStart = PqRecvPointer = PqRecvLength = 0;
	PqCommBusy = false;
	DoingCopyOut = false;
	on_proc_exit(socket_close, 0);
}