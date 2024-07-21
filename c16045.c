processOctetMsgLen(const instanceConf_t *const inst, struct conn_wrkr_s *connWrkr, char ch)
{
	DEFiRet;
	if (connWrkr->parseState.inputState == eAtStrtFram) {
		if (inst->bSuppOctetFram && isdigit(ch)) {
			connWrkr->parseState.inputState = eInOctetCnt;
			connWrkr->parseState.iOctetsRemain = 0;
			connWrkr->parseState.framingMode = TCP_FRAMING_OCTET_COUNTING;
		} else {
			connWrkr->parseState.inputState = eInMsg;
			connWrkr->parseState.framingMode = TCP_FRAMING_OCTET_STUFFING;
		}
	}

	// parsing character.
	if (connWrkr->parseState.inputState == eInOctetCnt) {
		if (isdigit(ch)) {
			if (connWrkr->parseState.iOctetsRemain <= 200000000) {
				connWrkr->parseState.iOctetsRemain = connWrkr->parseState.iOctetsRemain * 10 + ch - '0';
			}
			// temporarily save this character into the message buffer
			connWrkr->pMsg[connWrkr->iMsg++] = ch;
		} else {
			const char *remoteAddr = "";
			if (connWrkr->propRemoteAddr) {
				remoteAddr = (const char *)propGetSzStr(connWrkr->propRemoteAddr);
			}

			/* handle space delimeter */
			if (ch != ' ') {
				LogError(0, NO_ERRCODE, "Framing Error in received TCP message "
					"from peer: (ip) %s: to input: %s, delimiter is not "
					"SP but has ASCII value %d.",
					remoteAddr, inst->pszInputName, ch);
			}

			if (connWrkr->parseState.iOctetsRemain < 1) {
				LogError(0, NO_ERRCODE, "Framing Error in received TCP message"
					" from peer: (ip) %s: delimiter is not "
					"SP but has ASCII value %d.",
					remoteAddr, ch);
			} else if (connWrkr->parseState.iOctetsRemain > s_iMaxLine) {
				DBGPRINTF("truncating message with %lu octets - max msg size is %lu\n",
									connWrkr->parseState.iOctetsRemain, s_iMaxLine);
				LogError(0, NO_ERRCODE, "received oversize message from peer: "
					"(hostname) (ip) %s: size is %lu bytes, max msg "
					"size is %lu, truncating...",
					remoteAddr, connWrkr->parseState.iOctetsRemain, s_iMaxLine);
			}
			connWrkr->parseState.inputState = eInMsg;
		}
		/* reset msg len for actual message processing */
		connWrkr->iMsg = 0;
		/* retrieve next character */
	}
	RETiRet;
}