static void rtl_p2p_action_ie(struct ieee80211_hw *hw, void *data,
			      unsigned int len)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct ieee80211_mgmt *mgmt = data;
	struct rtl_p2p_ps_info *p2pinfo = &(rtlpriv->psc.p2p_ps_info);
	u8 noa_num, index , i , noa_index = 0;
	u8 *pos, *end, *ie;
	u16 noa_len;
	static u8 p2p_oui_ie_type[4] = {0x50, 0x6f, 0x9a, 0x09};

	pos = (u8 *)&mgmt->u.action.category;
	end = data + len;
	ie = NULL;

	if (pos[0] == 0x7f) {
		if (memcmp(&pos[1], p2p_oui_ie_type, 4) == 0)
			ie = pos + 3+4;
	}

	if (ie == NULL)
		return;

	RT_TRACE(rtlpriv, COMP_FW, DBG_LOUD, "action frame find P2P IE.\n");
	/*to find noa ie*/
	while (ie + 1 < end) {
		noa_len = READEF2BYTE((__le16 *)&ie[1]);
		if (ie + 3 + ie[1] > end)
			return;

		if (ie[0] == 12) {
			RT_TRACE(rtlpriv, COMP_FW, DBG_LOUD, "find NOA IE.\n");
			RT_PRINT_DATA(rtlpriv, COMP_FW, DBG_LOUD, "noa ie ",
				      ie, noa_len);
			if ((noa_len - 2) % 13 != 0) {
				RT_TRACE(rtlpriv, COMP_FW, DBG_LOUD,
					 "P2P notice of absence: invalid length.%d\n",
					 noa_len);
				return;
			} else {
				noa_num = (noa_len - 2) / 13;
			}
			noa_index = ie[3];
			if (rtlpriv->psc.p2p_ps_info.p2p_ps_mode ==
			    P2P_PS_NONE || noa_index != p2pinfo->noa_index) {
				p2pinfo->noa_index = noa_index;
				p2pinfo->opp_ps = (ie[4] >> 7);
				p2pinfo->ctwindow = ie[4] & 0x7F;
				p2pinfo->noa_num = noa_num;
				index = 5;
				for (i = 0; i < noa_num; i++) {
					p2pinfo->noa_count_type[i] =
							READEF1BYTE(ie+index);
					index += 1;
					p2pinfo->noa_duration[i] =
							 READEF4BYTE((__le32 *)ie+index);
					index += 4;
					p2pinfo->noa_interval[i] =
							 READEF4BYTE((__le32 *)ie+index);
					index += 4;
					p2pinfo->noa_start_time[i] =
							 READEF4BYTE((__le32 *)ie+index);
					index += 4;
				}

				if (p2pinfo->opp_ps == 1) {
					p2pinfo->p2p_ps_mode = P2P_PS_CTWINDOW;
					/* Driver should wait LPS entering
					 * CTWindow
					 */
					if (rtlpriv->psc.fw_current_inpsmode)
						rtl_p2p_ps_cmd(hw,
							       P2P_PS_ENABLE);
				} else if (p2pinfo->noa_num > 0) {
					p2pinfo->p2p_ps_mode = P2P_PS_NOA;
					rtl_p2p_ps_cmd(hw, P2P_PS_ENABLE);
				} else if (p2pinfo->p2p_ps_mode > P2P_PS_NONE) {
					rtl_p2p_ps_cmd(hw, P2P_PS_DISABLE);
				}
			}
			break;
		}
		ie += 3 + noa_len;
	}
}