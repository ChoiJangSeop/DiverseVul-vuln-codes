static int ceph_x_proc_ticket_reply(struct ceph_auth_client *ac,
				    struct ceph_crypto_key *secret,
				    void *buf, void *end)
{
	void *p = buf;
	char *dbuf;
	char *ticket_buf;
	u8 reply_struct_v;
	u32 num;
	int ret;

	dbuf = kmalloc(TEMP_TICKET_BUF_LEN, GFP_NOFS);
	if (!dbuf)
		return -ENOMEM;

	ret = -ENOMEM;
	ticket_buf = kmalloc(TEMP_TICKET_BUF_LEN, GFP_NOFS);
	if (!ticket_buf)
		goto out_dbuf;

	ceph_decode_8_safe(&p, end, reply_struct_v, bad);
	if (reply_struct_v != 1)
		return -EINVAL;

	ceph_decode_32_safe(&p, end, num, bad);
	dout("%d tickets\n", num);

	while (num--) {
		ret = process_one_ticket(ac, secret, &p, end,
					 dbuf, ticket_buf);
		if (ret)
			goto out;
	}

	ret = 0;
out:
	kfree(ticket_buf);
out_dbuf:
	kfree(dbuf);
	return ret;

bad:
	ret = -EINVAL;
	goto out;
}