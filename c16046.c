processDataRcvd(tcps_sess_t *pThis,
	const char c,
	struct syslogTime *stTime,
	const time_t ttGenTime,
	multi_submit_t *pMultiSub,
	unsigned *const __restrict__ pnMsgs)
{
	DEFiRet;
	const tcpLstnParams_t *const cnf_params = pThis->pLstnInfo->cnf_params;
	ISOBJ_TYPE_assert(pThis, tcps_sess);
	int iMaxLine = glbl.GetMaxLine(runConf);
	uchar *propPeerName = NULL;
	int lenPeerName = 0;
	uchar *propPeerIP = NULL;
	int lenPeerIP = 0;

	if(pThis->inputState == eAtStrtFram) {
		if(pThis->bSuppOctetFram && c >= '0' && c <= '9') {
			pThis->inputState = eInOctetCnt;
			pThis->iOctetsRemain = 0;
			pThis->eFraming = TCP_FRAMING_OCTET_COUNTING;
		} else if(pThis->bSPFramingFix && c == ' ') {
			/* Cisco ASA very occasionally sends a SP after a LF, which
			 * thrashes framing if not taken special care of. Here,
			 * we permit space *in front of the next frame* and
			 * ignore it.
			 */
			 FINALIZE;
		} else {
			pThis->inputState = eInMsg;
			pThis->eFraming = TCP_FRAMING_OCTET_STUFFING;
		}
	}

	if(pThis->inputState == eInOctetCnt) {
		if(c >= '0' && c <= '9') { /* isdigit() the faster way */
			if(pThis->iOctetsRemain <= 200000000) {
				pThis->iOctetsRemain = pThis->iOctetsRemain * 10 + c - '0';
			}
			*(pThis->pMsg + pThis->iMsg++) = c;
		} else { /* done with the octet count, so this must be the SP terminator */
			DBGPRINTF("TCP Message with octet-counter, size %d.\n", pThis->iOctetsRemain);
			prop.GetString(pThis->fromHost, &propPeerName, &lenPeerName);
			prop.GetString(pThis->fromHost, &propPeerIP, &lenPeerIP);
			if(c != ' ') {
				LogError(0, NO_ERRCODE, "imtcp %s: Framing Error in received TCP message from "
					"peer: (hostname) %s, (ip) %s: delimiter is not SP but has "
					"ASCII value %d.", cnf_params->pszInputName, propPeerName, propPeerIP, c);
			}
			if(pThis->iOctetsRemain < 1) {
				/* TODO: handle the case where the octet count is 0! */
				LogError(0, NO_ERRCODE, "imtcp %s: Framing Error in received TCP message from "
					"peer: (hostname) %s, (ip) %s: invalid octet count %d.",
					cnf_params->pszInputName, propPeerName, propPeerIP, pThis->iOctetsRemain);
				pThis->eFraming = TCP_FRAMING_OCTET_STUFFING;
			} else if(pThis->iOctetsRemain > iMaxLine) {
				/* while we can not do anything against it, we can at least log an indication
				 * that something went wrong) -- rgerhards, 2008-03-14
				 */
				LogError(0, NO_ERRCODE, "imtcp %s: received oversize message from peer: "
					"(hostname) %s, (ip) %s: size is %d bytes, max msg size "
					"is %d, truncating...", cnf_params->pszInputName, propPeerName,
					propPeerIP, pThis->iOctetsRemain, iMaxLine);
			}
			if(pThis->iOctetsRemain > pThis->pSrv->maxFrameSize) {
				LogError(0, NO_ERRCODE, "imtcp %s: Framing Error in received TCP message from "
					"peer: (hostname) %s, (ip) %s: frame too large: %d, change "
					"to octet stuffing", cnf_params->pszInputName, propPeerName, propPeerIP,
						pThis->iOctetsRemain);
				pThis->eFraming = TCP_FRAMING_OCTET_STUFFING;
			} else {
				pThis->iMsg = 0;
			}
			pThis->inputState = eInMsg;
		}
	} else if(pThis->inputState == eInMsgTruncating) {
		if(pThis->eFraming == TCP_FRAMING_OCTET_COUNTING) {
			DBGPRINTF("DEBUG: TCP_FRAMING_OCTET_COUNTING eInMsgTruncating c=%c remain=%d\n",
				c, pThis->iOctetsRemain);

			pThis->iOctetsRemain--;
			if(pThis->iOctetsRemain < 1) {
				pThis->inputState = eAtStrtFram;
			}
		} else {
			if(    ((c == '\n') && !pThis->pSrv->bDisableLFDelim)
			    || ((pThis->pSrv->addtlFrameDelim != TCPSRV_NO_ADDTL_DELIMITER)
			         && (c == pThis->pSrv->addtlFrameDelim))
			    ) {
				pThis->inputState = eAtStrtFram;
			}
		}
	} else {
		assert(pThis->inputState == eInMsg);
		#if 0 // set to 1 for ultra-verbose
		DBGPRINTF("DEBUG: processDataRcvd c=%c remain=%d\n",
			c, pThis->iOctetsRemain);
		#endif

		if((   ((c == '\n') && !pThis->pSrv->bDisableLFDelim)
		   || ((pThis->pSrv->addtlFrameDelim != TCPSRV_NO_ADDTL_DELIMITER)
		        && (c == pThis->pSrv->addtlFrameDelim))
		   ) && pThis->eFraming == TCP_FRAMING_OCTET_STUFFING) { /* record delimiter? */
			defaultDoSubmitMessage(pThis, stTime, ttGenTime, pMultiSub);
			++(*pnMsgs);
			pThis->inputState = eAtStrtFram;
		} else {
			/* IMPORTANT: here we copy the actual frame content to the message - for BOTH framing modes!
			 * If we have a message that is larger than the max msg size, we truncate it. This is the best
			 * we can do in light of what the engine supports. -- rgerhards, 2008-03-14
			 */
			if(pThis->iMsg < iMaxLine) {
				*(pThis->pMsg + pThis->iMsg++) = c;
			} else {
				/* emergency, we now need to flush, no matter if we are at end of message or not... */
				DBGPRINTF("error: message received is larger than max msg size, we %s it - c=%x\n",
					pThis->pSrv->discardTruncatedMsg == 1 ? "truncate" : "split", c);
				defaultDoSubmitMessage(pThis, stTime, ttGenTime, pMultiSub);
				++(*pnMsgs);
				if(pThis->pSrv->discardTruncatedMsg == 1) {
					if (pThis->eFraming == TCP_FRAMING_OCTET_COUNTING) {
						pThis->iOctetsRemain--;
						if (pThis->iOctetsRemain == 0) {
							pThis->inputState = eAtStrtFram;
							FINALIZE;
						}
					}
					pThis->inputState = eInMsgTruncating;
					FINALIZE;
				}
			}
		}

		if(pThis->eFraming == TCP_FRAMING_OCTET_COUNTING) {
			/* do we need to find end-of-frame via octet counting? */
			pThis->iOctetsRemain--;
			if(pThis->iOctetsRemain < 1) {
				/* we have end of frame! */
				defaultDoSubmitMessage(pThis, stTime, ttGenTime, pMultiSub);
				++(*pnMsgs);
				pThis->inputState = eAtStrtFram;
			}
		}
	}

finalize_it:
	RETiRet;
}