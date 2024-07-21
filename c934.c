static int unix_scm_to_skb(struct scm_cookie *scm, struct sk_buff *skb, bool send_fds)
{
	int err = 0;
	UNIXCB(skb).pid  = get_pid(scm->pid);
	UNIXCB(skb).cred = get_cred(scm->cred);
	UNIXCB(skb).fp = NULL;
	if (scm->fp && send_fds)
		err = unix_attach_fds(scm, skb);

	skb->destructor = unix_destruct_scm;
	return err;
}