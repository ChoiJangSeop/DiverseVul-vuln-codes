static int do_hidp_sock_ioctl(struct socket *sock, unsigned int cmd, void __user *argp)
{
	struct hidp_connadd_req ca;
	struct hidp_conndel_req cd;
	struct hidp_connlist_req cl;
	struct hidp_conninfo ci;
	struct socket *csock;
	struct socket *isock;
	int err;

	BT_DBG("cmd %x arg %p", cmd, argp);

	switch (cmd) {
	case HIDPCONNADD:
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		if (copy_from_user(&ca, argp, sizeof(ca)))
			return -EFAULT;

		csock = sockfd_lookup(ca.ctrl_sock, &err);
		if (!csock)
			return err;

		isock = sockfd_lookup(ca.intr_sock, &err);
		if (!isock) {
			sockfd_put(csock);
			return err;
		}

		err = hidp_connection_add(&ca, csock, isock);
		if (!err && copy_to_user(argp, &ca, sizeof(ca)))
			err = -EFAULT;

		sockfd_put(csock);
		sockfd_put(isock);

		return err;

	case HIDPCONNDEL:
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		if (copy_from_user(&cd, argp, sizeof(cd)))
			return -EFAULT;

		return hidp_connection_del(&cd);

	case HIDPGETCONNLIST:
		if (copy_from_user(&cl, argp, sizeof(cl)))
			return -EFAULT;

		if (cl.cnum <= 0)
			return -EINVAL;

		err = hidp_get_connlist(&cl);
		if (!err && copy_to_user(argp, &cl, sizeof(cl)))
			return -EFAULT;

		return err;

	case HIDPGETCONNINFO:
		if (copy_from_user(&ci, argp, sizeof(ci)))
			return -EFAULT;

		err = hidp_get_conninfo(&ci);
		if (!err && copy_to_user(argp, &ci, sizeof(ci)))
			return -EFAULT;

		return err;
	}

	return -EINVAL;
}