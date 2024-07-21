long ssl2_ctrl(SSL *s, int cmd, long larg, void *parg)
	{
	int ret=0;

	switch(cmd)
		{
	case SSL_CTRL_GET_SESSION_REUSED:
		ret=s->hit;
		break;
	default:
		break;
		}
	return(ret);
	}