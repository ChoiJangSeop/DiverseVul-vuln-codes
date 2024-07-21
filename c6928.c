rfbTightExtensionClientClose(rfbClientPtr cl, void* data) {

	if(data != NULL)
		free(data);

}