smb_send_kvec(struct TCP_Server_Info *server, struct kvec *iov, size_t n_vec,
		size_t *sent)
{
	int rc = 0;
	int i = 0;
	struct msghdr smb_msg;
	unsigned int remaining;
	size_t first_vec = 0;
	struct socket *ssocket = server->ssocket;

	*sent = 0;

	if (ssocket == NULL)
		return -ENOTSOCK; /* BB eventually add reconnect code here */

	smb_msg.msg_name = (struct sockaddr *) &server->dstaddr;
	smb_msg.msg_namelen = sizeof(struct sockaddr);
	smb_msg.msg_control = NULL;
	smb_msg.msg_controllen = 0;
	if (server->noblocksnd)
		smb_msg.msg_flags = MSG_DONTWAIT + MSG_NOSIGNAL;
	else
		smb_msg.msg_flags = MSG_NOSIGNAL;

	remaining = 0;
	for (i = 0; i < n_vec; i++)
		remaining += iov[i].iov_len;

	i = 0;
	while (remaining) {
		/*
		 * If blocking send, we try 3 times, since each can block
		 * for 5 seconds. For nonblocking  we have to try more
		 * but wait increasing amounts of time allowing time for
		 * socket to clear.  The overall time we wait in either
		 * case to send on the socket is about 15 seconds.
		 * Similarly we wait for 15 seconds for a response from
		 * the server in SendReceive[2] for the server to send
		 * a response back for most types of requests (except
		 * SMB Write past end of file which can be slow, and
		 * blocking lock operations). NFS waits slightly longer
		 * than CIFS, but this can make it take longer for
		 * nonresponsive servers to be detected and 15 seconds
		 * is more than enough time for modern networks to
		 * send a packet.  In most cases if we fail to send
		 * after the retries we will kill the socket and
		 * reconnect which may clear the network problem.
		 */
		rc = kernel_sendmsg(ssocket, &smb_msg, &iov[first_vec],
				    n_vec - first_vec, remaining);
		if (rc == -ENOSPC || rc == -EAGAIN) {
			/*
			 * Catch if a low level driver returns -ENOSPC. This
			 * WARN_ON will be removed by 3.10 if no one reports
			 * seeing this.
			 */
			WARN_ON_ONCE(rc == -ENOSPC);
			i++;
			if (i >= 14 || (!server->noblocksnd && (i > 2))) {
				cERROR(1, "sends on sock %p stuck for 15 "
					  "seconds", ssocket);
				rc = -EAGAIN;
				break;
			}
			msleep(1 << i);
			continue;
		}

		if (rc < 0)
			break;

		/* send was at least partially successful */
		*sent += rc;

		if (rc == remaining) {
			remaining = 0;
			break;
		}

		if (rc > remaining) {
			cERROR(1, "sent %d requested %d", rc, remaining);
			break;
		}

		if (rc == 0) {
			/* should never happen, letting socket clear before
			   retrying is our only obvious option here */
			cERROR(1, "tcp sent no data");
			msleep(500);
			continue;
		}

		remaining -= rc;

		/* the line below resets i */
		for (i = first_vec; i < n_vec; i++) {
			if (iov[i].iov_len) {
				if (rc > iov[i].iov_len) {
					rc -= iov[i].iov_len;
					iov[i].iov_len = 0;
				} else {
					iov[i].iov_base += rc;
					iov[i].iov_len -= rc;
					first_vec = i;
					break;
				}
			}
		}

		i = 0; /* in case we get ENOSPC on the next send */
		rc = 0;
	}
	return rc;
}