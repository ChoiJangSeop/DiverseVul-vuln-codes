CloseUndoneFileDownload(rfbClientPtr cl, rfbTightClientPtr rtcp)
{
	if(cl == NULL)
		return;
	
	if(rtcp->rcft.rcfd.downloadInProgress == TRUE) {
		rtcp->rcft.rcfd.downloadInProgress = FALSE;

		if(rtcp->rcft.rcfd.downloadFD != -1) {			
			close(rtcp->rcft.rcfd.downloadFD);
			rtcp->rcft.rcfd.downloadFD = -1;
		}
		memset(rtcp->rcft.rcfd.fName, 0 , PATH_MAX);
	}
}