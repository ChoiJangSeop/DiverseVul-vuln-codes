static long ppp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct ppp_file *pf = file->private_data;
	struct ppp *ppp;
	int err = -EFAULT, val, val2, i;
	struct ppp_idle idle;
	struct npioctl npi;
	int unit, cflags;
	struct slcompress *vj;
	void __user *argp = (void __user *)arg;
	int __user *p = argp;

	if (!pf)
		return ppp_unattached_ioctl(current->nsproxy->net_ns,
					pf, file, cmd, arg);

	if (cmd == PPPIOCDETACH) {
		/*
		 * We have to be careful here... if the file descriptor
		 * has been dup'd, we could have another process in the
		 * middle of a poll using the same file *, so we had
		 * better not free the interface data structures -
		 * instead we fail the ioctl.  Even in this case, we
		 * shut down the interface if we are the owner of it.
		 * Actually, we should get rid of PPPIOCDETACH, userland
		 * (i.e. pppd) could achieve the same effect by closing
		 * this fd and reopening /dev/ppp.
		 */
		err = -EINVAL;
		mutex_lock(&ppp_mutex);
		if (pf->kind == INTERFACE) {
			ppp = PF_TO_PPP(pf);
			rtnl_lock();
			if (file == ppp->owner)
				unregister_netdevice(ppp->dev);
			rtnl_unlock();
		}
		if (atomic_long_read(&file->f_count) < 2) {
			ppp_release(NULL, file);
			err = 0;
		} else
			pr_warn("PPPIOCDETACH file->f_count=%ld\n",
				atomic_long_read(&file->f_count));
		mutex_unlock(&ppp_mutex);
		return err;
	}

	if (pf->kind == CHANNEL) {
		struct channel *pch;
		struct ppp_channel *chan;

		mutex_lock(&ppp_mutex);
		pch = PF_TO_CHANNEL(pf);

		switch (cmd) {
		case PPPIOCCONNECT:
			if (get_user(unit, p))
				break;
			err = ppp_connect_channel(pch, unit);
			break;

		case PPPIOCDISCONN:
			err = ppp_disconnect_channel(pch);
			break;

		default:
			down_read(&pch->chan_sem);
			chan = pch->chan;
			err = -ENOTTY;
			if (chan && chan->ops->ioctl)
				err = chan->ops->ioctl(chan, cmd, arg);
			up_read(&pch->chan_sem);
		}
		mutex_unlock(&ppp_mutex);
		return err;
	}

	if (pf->kind != INTERFACE) {
		/* can't happen */
		pr_err("PPP: not interface or channel??\n");
		return -EINVAL;
	}

	mutex_lock(&ppp_mutex);
	ppp = PF_TO_PPP(pf);
	switch (cmd) {
	case PPPIOCSMRU:
		if (get_user(val, p))
			break;
		ppp->mru = val;
		err = 0;
		break;

	case PPPIOCSFLAGS:
		if (get_user(val, p))
			break;
		ppp_lock(ppp);
		cflags = ppp->flags & ~val;
#ifdef CONFIG_PPP_MULTILINK
		if (!(ppp->flags & SC_MULTILINK) && (val & SC_MULTILINK))
			ppp->nextseq = 0;
#endif
		ppp->flags = val & SC_FLAG_BITS;
		ppp_unlock(ppp);
		if (cflags & SC_CCP_OPEN)
			ppp_ccp_closed(ppp);
		err = 0;
		break;

	case PPPIOCGFLAGS:
		val = ppp->flags | ppp->xstate | ppp->rstate;
		if (put_user(val, p))
			break;
		err = 0;
		break;

	case PPPIOCSCOMPRESS:
		err = ppp_set_compress(ppp, arg);
		break;

	case PPPIOCGUNIT:
		if (put_user(ppp->file.index, p))
			break;
		err = 0;
		break;

	case PPPIOCSDEBUG:
		if (get_user(val, p))
			break;
		ppp->debug = val;
		err = 0;
		break;

	case PPPIOCGDEBUG:
		if (put_user(ppp->debug, p))
			break;
		err = 0;
		break;

	case PPPIOCGIDLE:
		idle.xmit_idle = (jiffies - ppp->last_xmit) / HZ;
		idle.recv_idle = (jiffies - ppp->last_recv) / HZ;
		if (copy_to_user(argp, &idle, sizeof(idle)))
			break;
		err = 0;
		break;

	case PPPIOCSMAXCID:
		if (get_user(val, p))
			break;
		val2 = 15;
		if ((val >> 16) != 0) {
			val2 = val >> 16;
			val &= 0xffff;
		}
		vj = slhc_init(val2+1, val+1);
		if (!vj) {
			netdev_err(ppp->dev,
				   "PPP: no memory (VJ compressor)\n");
			err = -ENOMEM;
			break;
		}
		ppp_lock(ppp);
		if (ppp->vj)
			slhc_free(ppp->vj);
		ppp->vj = vj;
		ppp_unlock(ppp);
		err = 0;
		break;

	case PPPIOCGNPMODE:
	case PPPIOCSNPMODE:
		if (copy_from_user(&npi, argp, sizeof(npi)))
			break;
		err = proto_to_npindex(npi.protocol);
		if (err < 0)
			break;
		i = err;
		if (cmd == PPPIOCGNPMODE) {
			err = -EFAULT;
			npi.mode = ppp->npmode[i];
			if (copy_to_user(argp, &npi, sizeof(npi)))
				break;
		} else {
			ppp->npmode[i] = npi.mode;
			/* we may be able to transmit more packets now (??) */
			netif_wake_queue(ppp->dev);
		}
		err = 0;
		break;

#ifdef CONFIG_PPP_FILTER
	case PPPIOCSPASS:
	{
		struct sock_filter *code;

		err = get_filter(argp, &code);
		if (err >= 0) {
			struct bpf_prog *pass_filter = NULL;
			struct sock_fprog_kern fprog = {
				.len = err,
				.filter = code,
			};

			err = 0;
			if (fprog.filter)
				err = bpf_prog_create(&pass_filter, &fprog);
			if (!err) {
				ppp_lock(ppp);
				if (ppp->pass_filter)
					bpf_prog_destroy(ppp->pass_filter);
				ppp->pass_filter = pass_filter;
				ppp_unlock(ppp);
			}
			kfree(code);
		}
		break;
	}
	case PPPIOCSACTIVE:
	{
		struct sock_filter *code;

		err = get_filter(argp, &code);
		if (err >= 0) {
			struct bpf_prog *active_filter = NULL;
			struct sock_fprog_kern fprog = {
				.len = err,
				.filter = code,
			};

			err = 0;
			if (fprog.filter)
				err = bpf_prog_create(&active_filter, &fprog);
			if (!err) {
				ppp_lock(ppp);
				if (ppp->active_filter)
					bpf_prog_destroy(ppp->active_filter);
				ppp->active_filter = active_filter;
				ppp_unlock(ppp);
			}
			kfree(code);
		}
		break;
	}
#endif /* CONFIG_PPP_FILTER */

#ifdef CONFIG_PPP_MULTILINK
	case PPPIOCSMRRU:
		if (get_user(val, p))
			break;
		ppp_recv_lock(ppp);
		ppp->mrru = val;
		ppp_recv_unlock(ppp);
		err = 0;
		break;
#endif /* CONFIG_PPP_MULTILINK */

	default:
		err = -ENOTTY;
	}
	mutex_unlock(&ppp_mutex);
	return err;
}