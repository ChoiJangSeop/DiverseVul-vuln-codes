long dtls1_ctrl(SSL *s, int cmd, long larg, void *parg)
	{
	int ret=0;

	switch (cmd)
		{
	case DTLS_CTRL_GET_TIMEOUT:
		if (dtls1_get_timeout(s, (struct timeval*) parg) != NULL)
			{
			ret = 1;
			}
		break;
	case DTLS_CTRL_HANDLE_TIMEOUT:
		ret = dtls1_handle_timeout(s);
		break;
	case DTLS_CTRL_LISTEN:
		ret = dtls1_listen(s, parg);
		break;

	default:
		ret = ssl3_ctrl(s, cmd, larg, parg);
		break;
		}
	return(ret);
	}