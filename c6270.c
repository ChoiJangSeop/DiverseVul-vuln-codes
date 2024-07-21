SanitizeMsg(smsg_t *pMsg)
{
	DEFiRet;
	uchar *pszMsg;
	uchar *pDst; /* destination for copy job */
	size_t lenMsg;
	size_t iSrc;
	size_t iDst;
	size_t iMaxLine;
	size_t maxDest;
	uchar pc;
	sbool bUpdatedLen = RSFALSE;
	uchar szSanBuf[32*1024]; /* buffer used for sanitizing a string */

	assert(pMsg != NULL);
	assert(pMsg->iLenRawMsg > 0);

	pszMsg = pMsg->pszRawMsg;
	lenMsg = pMsg->iLenRawMsg;

	/* remove NUL character at end of message (see comment in function header)
	 * Note that we do not need to add a NUL character in this case, because it
	 * is already present ;)
	 */
	if(pszMsg[lenMsg-1] == '\0') {
		DBGPRINTF("dropped NUL at very end of message\n");
		bUpdatedLen = RSTRUE;
		lenMsg--;
	}

	/* then we check if we need to drop trailing LFs, which often make
	 * their way into syslog messages unintentionally. In order to remain
	 * compatible to recent IETF developments, we allow the user to
	 * turn on/off this handling.  rgerhards, 2007-07-23
	 */
	if(glbl.GetParserDropTrailingLFOnReception()
	   && lenMsg > 0 && pszMsg[lenMsg-1] == '\n') {
		DBGPRINTF("dropped LF at very end of message (DropTrailingLF is set)\n");
		lenMsg--;
		pszMsg[lenMsg] = '\0';
		bUpdatedLen = RSTRUE;
	}

	/* it is much quicker to sweep over the message and see if it actually
	 * needs sanitation than to do the sanitation in any case. So we first do
	 * this and terminate when it is not needed - which is expectedly the case
	 * for the vast majority of messages. -- rgerhards, 2009-06-15
	 * Note that we do NOT check here if tab characters are to be escaped or
	 * not. I expect this functionality to be seldomly used and thus I do not
	 * like to pay the performance penalty. So the penalty is only with those
	 * that actually use it, because we may call the sanitizer without actual
	 * need below (but it then still will work perfectly well!). -- rgerhards, 2009-11-27
	 */
	int bNeedSanitize = 0;
	for(iSrc = 0 ; iSrc < lenMsg ; iSrc++) {
		if(pszMsg[iSrc] < 32) {
			if(glbl.GetParserSpaceLFOnReceive() && pszMsg[iSrc] == '\n') {
				pszMsg[iSrc] = ' ';
			} else if(pszMsg[iSrc] == '\0' || glbl.GetParserEscapeControlCharactersOnReceive()) {
				bNeedSanitize = 1;
				if (!glbl.GetParserSpaceLFOnReceive()) {
					break;
			    }
			}
		} else if(pszMsg[iSrc] > 127 && glbl.GetParserEscape8BitCharactersOnReceive()) {
			bNeedSanitize = 1;
			break;
		}
	}

	if(!bNeedSanitize) {
		if(bUpdatedLen == RSTRUE)
			MsgSetRawMsgSize(pMsg, lenMsg);
		FINALIZE;
	}

	/* now copy over the message and sanitize it. Note that up to iSrc-1 there was
	 * obviously no need to sanitize, so we can go over that quickly...
	 */
	iMaxLine = glbl.GetMaxLine();
	maxDest = lenMsg * 4; /* message can grow at most four-fold */

	if(maxDest > iMaxLine)
		maxDest = iMaxLine;	/* but not more than the max size! */
	if(maxDest < sizeof(szSanBuf))
		pDst = szSanBuf;
	else 
		CHKmalloc(pDst = MALLOC(iMaxLine + 1));
	if(iSrc > 0) {
		iSrc--; /* go back to where everything is OK */
		memcpy(pDst, pszMsg, iSrc); /* fast copy known good */
	}
	iDst = iSrc;
	while(iSrc < lenMsg && iDst < maxDest - 3) { /* leave some space if last char must be escaped */
		if((pszMsg[iSrc] < 32) && (pszMsg[iSrc] != '\t' || glbl.GetParserEscapeControlCharacterTab())) {
			/* note: \0 must always be escaped, the rest of the code currently
			 * can not handle it! -- rgerhards, 2009-08-26
			 */
			if(pszMsg[iSrc] == '\0' || glbl.GetParserEscapeControlCharactersOnReceive()) {
				/* we are configured to escape control characters. Please note
				 * that this most probably break non-western character sets like
				 * Japanese, Korean or Chinese. rgerhards, 2007-07-17
				 */
				if (glbl.GetParserEscapeControlCharactersCStyle()) {
					pDst[iDst++] = '\\';

					switch (pszMsg[iSrc]) {
					case '\0':
						pDst[iDst++] = '0';
						break;
					case '\a':
						pDst[iDst++] = 'a';
						break;
					case '\b':
						pDst[iDst++] = 'b';
						break;
					case '\e':
						pDst[iDst++] = 'e';
						break;
					case '\f':
						pDst[iDst++] = 'f';
						break;
					case '\n':
						pDst[iDst++] = 'n';
						break;
					case '\r':
						pDst[iDst++] = 'r';
						break;
					case '\t':
						pDst[iDst++] = 't';
						break;
					case '\v':
						pDst[iDst++] = 'v';
						break;
					default:
						pDst[iDst++] = 'x';

						pc = pszMsg[iSrc];
						pDst[iDst++] = hexdigit[(pc & 0xF0) >> 4];
						pDst[iDst++] = hexdigit[pc & 0xF];

						break;
					}

				} else {
					pDst[iDst++] = glbl.GetParserControlCharacterEscapePrefix();
					pDst[iDst++] = '0' + ((pszMsg[iSrc] & 0300) >> 6);
					pDst[iDst++] = '0' + ((pszMsg[iSrc] & 0070) >> 3);
					pDst[iDst++] = '0' + ((pszMsg[iSrc] & 0007));
				}
			}

		} else if(pszMsg[iSrc] > 127 && glbl.GetParserEscape8BitCharactersOnReceive()) {
			if (glbl.GetParserEscapeControlCharactersCStyle()) {
				pDst[iDst++] = '\\';
				pDst[iDst++] = 'x';

				pc = pszMsg[iSrc];
				pDst[iDst++] = hexdigit[(pc & 0xF0) >> 4];
				pDst[iDst++] = hexdigit[pc & 0xF];

			} else {
				/* In this case, we also do the conversion. Note that this most
				 * probably breaks European languages. -- rgerhards, 2010-01-27
				 */
				pDst[iDst++] = glbl.GetParserControlCharacterEscapePrefix();
				pDst[iDst++] = '0' + ((pszMsg[iSrc] & 0300) >> 6);
				pDst[iDst++] = '0' + ((pszMsg[iSrc] & 0070) >> 3);
				pDst[iDst++] = '0' + ((pszMsg[iSrc] & 0007));
			}
		} else {
			pDst[iDst++] = pszMsg[iSrc];
		}
		++iSrc;
	}
	pDst[iDst] = '\0';

	MsgSetRawMsg(pMsg, (char*)pDst, iDst); /* save sanitized string */

	if(pDst != szSanBuf)
		free(pDst);

finalize_it:
	RETiRet;
}