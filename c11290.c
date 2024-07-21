static int ixgbe_rcv_msg_from_vf(struct ixgbe_adapter *adapter, u32 vf)
{
	u32 mbx_size = IXGBE_VFMAILBOX_SIZE;
	u32 msgbuf[IXGBE_VFMAILBOX_SIZE];
	struct ixgbe_hw *hw = &adapter->hw;
	s32 retval;

	retval = ixgbe_read_mbx(hw, msgbuf, mbx_size, vf);

	if (retval) {
		pr_err("Error receiving message from VF\n");
		return retval;
	}

	/* this is a message we already processed, do nothing */
	if (msgbuf[0] & (IXGBE_VT_MSGTYPE_ACK | IXGBE_VT_MSGTYPE_NACK))
		return 0;

	/* flush the ack before we write any messages back */
	IXGBE_WRITE_FLUSH(hw);

	if (msgbuf[0] == IXGBE_VF_RESET)
		return ixgbe_vf_reset_msg(adapter, vf);

	/*
	 * until the vf completes a virtual function reset it should not be
	 * allowed to start any configuration.
	 */
	if (!adapter->vfinfo[vf].clear_to_send) {
		msgbuf[0] |= IXGBE_VT_MSGTYPE_NACK;
		ixgbe_write_mbx(hw, msgbuf, 1, vf);
		return 0;
	}

	switch ((msgbuf[0] & 0xFFFF)) {
	case IXGBE_VF_SET_MAC_ADDR:
		retval = ixgbe_set_vf_mac_addr(adapter, msgbuf, vf);
		break;
	case IXGBE_VF_SET_MULTICAST:
		retval = ixgbe_set_vf_multicasts(adapter, msgbuf, vf);
		break;
	case IXGBE_VF_SET_VLAN:
		retval = ixgbe_set_vf_vlan_msg(adapter, msgbuf, vf);
		break;
	case IXGBE_VF_SET_LPE:
		retval = ixgbe_set_vf_lpe(adapter, msgbuf, vf);
		break;
	case IXGBE_VF_SET_MACVLAN:
		retval = ixgbe_set_vf_macvlan_msg(adapter, msgbuf, vf);
		break;
	case IXGBE_VF_API_NEGOTIATE:
		retval = ixgbe_negotiate_vf_api(adapter, msgbuf, vf);
		break;
	case IXGBE_VF_GET_QUEUES:
		retval = ixgbe_get_vf_queues(adapter, msgbuf, vf);
		break;
	case IXGBE_VF_GET_RETA:
		retval = ixgbe_get_vf_reta(adapter, msgbuf, vf);
		break;
	case IXGBE_VF_GET_RSS_KEY:
		retval = ixgbe_get_vf_rss_key(adapter, msgbuf, vf);
		break;
	case IXGBE_VF_UPDATE_XCAST_MODE:
		retval = ixgbe_update_vf_xcast_mode(adapter, msgbuf, vf);
		break;
	case IXGBE_VF_IPSEC_ADD:
		retval = ixgbe_ipsec_vf_add_sa(adapter, msgbuf, vf);
		break;
	case IXGBE_VF_IPSEC_DEL:
		retval = ixgbe_ipsec_vf_del_sa(adapter, msgbuf, vf);
		break;
	default:
		e_err(drv, "Unhandled Msg %8.8x\n", msgbuf[0]);
		retval = IXGBE_ERR_MBX;
		break;
	}

	/* notify the VF of the results of what it sent us */
	if (retval)
		msgbuf[0] |= IXGBE_VT_MSGTYPE_NACK;
	else
		msgbuf[0] |= IXGBE_VT_MSGTYPE_ACK;

	msgbuf[0] |= IXGBE_VT_MSGTYPE_CTS;

	ixgbe_write_mbx(hw, msgbuf, mbx_size, vf);

	return retval;
}