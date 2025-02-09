long dgnc_mgmt_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long flags;
	void __user *uarg = (void __user *)arg;

	switch (cmd) {
	case DIGI_GETDD:
	{
		/*
		 * This returns the total number of boards
		 * in the system, as well as driver version
		 * and has space for a reserved entry
		 */
		struct digi_dinfo ddi;

		spin_lock_irqsave(&dgnc_global_lock, flags);

		ddi.dinfo_nboards = dgnc_NumBoards;
		sprintf(ddi.dinfo_version, "%s", DG_PART);

		spin_unlock_irqrestore(&dgnc_global_lock, flags);

		if (copy_to_user(uarg, &ddi, sizeof(ddi)))
			return -EFAULT;

		break;
	}

	case DIGI_GETBD:
	{
		int brd;

		struct digi_info di;

		if (copy_from_user(&brd, uarg, sizeof(int)))
			return -EFAULT;

		if (brd < 0 || brd >= dgnc_NumBoards)
			return -ENODEV;

		memset(&di, 0, sizeof(di));

		di.info_bdnum = brd;

		spin_lock_irqsave(&dgnc_Board[brd]->bd_lock, flags);

		di.info_bdtype = dgnc_Board[brd]->dpatype;
		di.info_bdstate = dgnc_Board[brd]->dpastatus;
		di.info_ioport = 0;
		di.info_physaddr = (ulong)dgnc_Board[brd]->membase;
		di.info_physsize = (ulong)dgnc_Board[brd]->membase
			- dgnc_Board[brd]->membase_end;
		if (dgnc_Board[brd]->state != BOARD_FAILED)
			di.info_nports = dgnc_Board[brd]->nasync;
		else
			di.info_nports = 0;

		spin_unlock_irqrestore(&dgnc_Board[brd]->bd_lock, flags);

		if (copy_to_user(uarg, &di, sizeof(di)))
			return -EFAULT;

		break;
	}

	case DIGI_GET_NI_INFO:
	{
		struct channel_t *ch;
		struct ni_info ni;
		unsigned char mstat = 0;
		uint board = 0;
		uint channel = 0;

		if (copy_from_user(&ni, uarg, sizeof(ni)))
			return -EFAULT;

		board = ni.board;
		channel = ni.channel;

		/* Verify boundaries on board */
		if (board >= dgnc_NumBoards)
			return -ENODEV;

		/* Verify boundaries on channel */
		if (channel >= dgnc_Board[board]->nasync)
			return -ENODEV;

		ch = dgnc_Board[board]->channels[channel];

		if (!ch || ch->magic != DGNC_CHANNEL_MAGIC)
			return -ENODEV;

		memset(&ni, 0, sizeof(ni));
		ni.board = board;
		ni.channel = channel;

		spin_lock_irqsave(&ch->ch_lock, flags);

		mstat = (ch->ch_mostat | ch->ch_mistat);

		if (mstat & UART_MCR_DTR) {
			ni.mstat |= TIOCM_DTR;
			ni.dtr = TIOCM_DTR;
		}
		if (mstat & UART_MCR_RTS) {
			ni.mstat |= TIOCM_RTS;
			ni.rts = TIOCM_RTS;
		}
		if (mstat & UART_MSR_CTS) {
			ni.mstat |= TIOCM_CTS;
			ni.cts = TIOCM_CTS;
		}
		if (mstat & UART_MSR_RI) {
			ni.mstat |= TIOCM_RI;
			ni.ri = TIOCM_RI;
		}
		if (mstat & UART_MSR_DCD) {
			ni.mstat |= TIOCM_CD;
			ni.dcd = TIOCM_CD;
		}
		if (mstat & UART_MSR_DSR)
			ni.mstat |= TIOCM_DSR;

		ni.iflag = ch->ch_c_iflag;
		ni.oflag = ch->ch_c_oflag;
		ni.cflag = ch->ch_c_cflag;
		ni.lflag = ch->ch_c_lflag;

		if (ch->ch_digi.digi_flags & CTSPACE ||
		    ch->ch_c_cflag & CRTSCTS)
			ni.hflow = 1;
		else
			ni.hflow = 0;

		if ((ch->ch_flags & CH_STOPI) ||
		    (ch->ch_flags & CH_FORCED_STOPI))
			ni.recv_stopped = 1;
		else
			ni.recv_stopped = 0;

		if ((ch->ch_flags & CH_STOP) || (ch->ch_flags & CH_FORCED_STOP))
			ni.xmit_stopped = 1;
		else
			ni.xmit_stopped = 0;

		ni.curtx = ch->ch_txcount;
		ni.currx = ch->ch_rxcount;

		ni.baud = ch->ch_old_baud;

		spin_unlock_irqrestore(&ch->ch_lock, flags);

		if (copy_to_user(uarg, &ni, sizeof(ni)))
			return -EFAULT;

		break;
	}
	}

	return 0;
}