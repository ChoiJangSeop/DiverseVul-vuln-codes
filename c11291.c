static s32 ixgbe_set_vf_lpe(struct ixgbe_adapter *adapter, u32 *msgbuf, u32 vf)
{
	struct ixgbe_hw *hw = &adapter->hw;
	int max_frame = msgbuf[1];
	u32 max_frs;

	/*
	 * For 82599EB we have to keep all PFs and VFs operating with
	 * the same max_frame value in order to avoid sending an oversize
	 * frame to a VF.  In order to guarantee this is handled correctly
	 * for all cases we have several special exceptions to take into
	 * account before we can enable the VF for receive
	 */
	if (adapter->hw.mac.type == ixgbe_mac_82599EB) {
		struct net_device *dev = adapter->netdev;
		int pf_max_frame = dev->mtu + ETH_HLEN;
		u32 reg_offset, vf_shift, vfre;
		s32 err = 0;

#ifdef CONFIG_FCOE
		if (dev->features & NETIF_F_FCOE_MTU)
			pf_max_frame = max_t(int, pf_max_frame,
					     IXGBE_FCOE_JUMBO_FRAME_SIZE);

#endif /* CONFIG_FCOE */
		switch (adapter->vfinfo[vf].vf_api) {
		case ixgbe_mbox_api_11:
		case ixgbe_mbox_api_12:
		case ixgbe_mbox_api_13:
		case ixgbe_mbox_api_14:
			/* Version 1.1 supports jumbo frames on VFs if PF has
			 * jumbo frames enabled which means legacy VFs are
			 * disabled
			 */
			if (pf_max_frame > ETH_FRAME_LEN)
				break;
			fallthrough;
		default:
			/* If the PF or VF are running w/ jumbo frames enabled
			 * we need to shut down the VF Rx path as we cannot
			 * support jumbo frames on legacy VFs
			 */
			if ((pf_max_frame > ETH_FRAME_LEN) ||
			    (max_frame > (ETH_FRAME_LEN + ETH_FCS_LEN)))
				err = -EINVAL;
			break;
		}

		/* determine VF receive enable location */
		vf_shift = vf % 32;
		reg_offset = vf / 32;

		/* enable or disable receive depending on error */
		vfre = IXGBE_READ_REG(hw, IXGBE_VFRE(reg_offset));
		if (err)
			vfre &= ~BIT(vf_shift);
		else
			vfre |= BIT(vf_shift);
		IXGBE_WRITE_REG(hw, IXGBE_VFRE(reg_offset), vfre);

		if (err) {
			e_err(drv, "VF max_frame %d out of range\n", max_frame);
			return err;
		}
	}

	/* MTU < 68 is an error and causes problems on some kernels */
	if (max_frame > IXGBE_MAX_JUMBO_FRAME_SIZE) {
		e_err(drv, "VF max_frame %d out of range\n", max_frame);
		return -EINVAL;
	}

	/* pull current max frame size from hardware */
	max_frs = IXGBE_READ_REG(hw, IXGBE_MAXFRS);
	max_frs &= IXGBE_MHADD_MFS_MASK;
	max_frs >>= IXGBE_MHADD_MFS_SHIFT;

	if (max_frs < max_frame) {
		max_frs = max_frame << IXGBE_MHADD_MFS_SHIFT;
		IXGBE_WRITE_REG(hw, IXGBE_MAXFRS, max_frs);
	}

	e_info(hw, "VF requests change max MTU to %d\n", max_frame);

	return 0;
}