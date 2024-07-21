	while(1) {
		/* Add the Unix Domain Sockets to the list of read
		 * descriptors.
		 * rgerhards 2005-08-01: we must now check if there are
		 * any local sockets to listen to at all. If the -o option
		 * is given without -a, we do not need to listen at all..
		 */
	        maxfds = 0;
	        FD_ZERO (&readfds);

		/* Add the UDP listen sockets to the list of read descriptors.
		 */
		if(udpLstnSocks != NULL) {
                        for (i = 0; i < *udpLstnSocks; i++) {
                                if (udpLstnSocks[i+1] != -1) {
					if(Debug)
						net.debugListenInfo(udpLstnSocks[i+1], "UDP");
                                        FD_SET(udpLstnSocks[i+1], &readfds);
					if(udpLstnSocks[i+1]>maxfds) maxfds=udpLstnSocks[i+1];
				}
                        }
		}
		if(Debug) {
			dbgprintf("--------imUDP calling select, active file descriptors (max %d): ", maxfds);
			for (nfds = 0; nfds <= maxfds; ++nfds)
				if ( FD_ISSET(nfds, &readfds) )
					dbgprintf("%d ", nfds);
			dbgprintf("\n");
		}

		/* wait for io to become ready */
		nfds = select(maxfds+1, (fd_set *) &readfds, NULL, NULL, NULL);

		if(udpLstnSocks != NULL) {
		       for (i = 0; nfds && i < *udpLstnSocks; i++) {
			       if (FD_ISSET(udpLstnSocks[i+1], &readfds)) {
				       socklen = sizeof(frominet);
				       l = recvfrom(udpLstnSocks[i+1], (char*) pRcvBuf, MAXLINE - 1, 0,
						    (struct sockaddr *)&frominet, &socklen);
				       if (l > 0) {
					       if(net.cvthname(&frominet, fromHost, fromHostFQDN, fromHostIP) == RS_RET_OK) {
						       dbgprintf("Message from inetd socket: #%d, host: %s\n",
							       udpLstnSocks[i+1], fromHost);
						       /* Here we check if a host is permitted to send us
							* syslog messages. If it isn't, we do not further
							* process the message but log a warning (if we are
							* configured to do this).
							* rgerhards, 2005-09-26
							*/
						       if(net.isAllowedSender((uchar*) "UDP",
							  (struct sockaddr *)&frominet, (char*)fromHostFQDN)) {
							       parseAndSubmitMessage(fromHost, fromHostIP, pRcvBuf, l,
							       MSG_PARSE_HOSTNAME, NOFLAG, eFLOWCTL_NO_DELAY);
						       } else {
							       dbgprintf("%s is not an allowed sender\n", (char*)fromHostFQDN);
							       if(glbl.GetOption_DisallowWarning) {
								       errmsg.LogError(0, NO_ERRCODE, "UDP message from disallowed sender %s discarded",
										  (char*)fromHost);
							       }	
						       }
					       }
				       } else if (l < 0 && errno != EINTR && errno != EAGAIN) {
						char errStr[1024];
						rs_strerror_r(errno, errStr, sizeof(errStr));
						dbgprintf("INET socket error: %d = %s.\n", errno, errStr);
						       errmsg.LogError(errno, NO_ERRCODE, "recvfrom inet");
						       /* should be harmless */
						       sleep(1);
					       }
					--nfds; /* indicate we have processed one */
				}
		       }
		}
	}