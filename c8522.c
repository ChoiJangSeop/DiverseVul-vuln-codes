static int ieee80211_change_station(struct wiphy *wiphy,
				    struct net_device *dev, const u8 *mac,
				    struct station_parameters *params)
{
	struct ieee80211_sub_if_data *sdata = IEEE80211_DEV_TO_SUB_IF(dev);
	struct ieee80211_local *local = wiphy_priv(wiphy);
	struct sta_info *sta;
	struct ieee80211_sub_if_data *vlansdata;
	enum cfg80211_station_type statype;
	int err;

	mutex_lock(&local->sta_mtx);

	sta = sta_info_get_bss(sdata, mac);
	if (!sta) {
		err = -ENOENT;
		goto out_err;
	}

	switch (sdata->vif.type) {
	case NL80211_IFTYPE_MESH_POINT:
		if (sdata->u.mesh.user_mpm)
			statype = CFG80211_STA_MESH_PEER_USER;
		else
			statype = CFG80211_STA_MESH_PEER_KERNEL;
		break;
	case NL80211_IFTYPE_ADHOC:
		statype = CFG80211_STA_IBSS;
		break;
	case NL80211_IFTYPE_STATION:
		if (!test_sta_flag(sta, WLAN_STA_TDLS_PEER)) {
			statype = CFG80211_STA_AP_STA;
			break;
		}
		if (test_sta_flag(sta, WLAN_STA_AUTHORIZED))
			statype = CFG80211_STA_TDLS_PEER_ACTIVE;
		else
			statype = CFG80211_STA_TDLS_PEER_SETUP;
		break;
	case NL80211_IFTYPE_AP:
	case NL80211_IFTYPE_AP_VLAN:
		if (test_sta_flag(sta, WLAN_STA_ASSOC))
			statype = CFG80211_STA_AP_CLIENT;
		else
			statype = CFG80211_STA_AP_CLIENT_UNASSOC;
		break;
	default:
		err = -EOPNOTSUPP;
		goto out_err;
	}

	err = cfg80211_check_station_change(wiphy, params, statype);
	if (err)
		goto out_err;

	if (params->vlan && params->vlan != sta->sdata->dev) {
		vlansdata = IEEE80211_DEV_TO_SUB_IF(params->vlan);

		if (params->vlan->ieee80211_ptr->use_4addr) {
			if (vlansdata->u.vlan.sta) {
				err = -EBUSY;
				goto out_err;
			}

			rcu_assign_pointer(vlansdata->u.vlan.sta, sta);
			__ieee80211_check_fast_rx_iface(vlansdata);
		}

		if (sta->sdata->vif.type == NL80211_IFTYPE_AP_VLAN &&
		    sta->sdata->u.vlan.sta)
			RCU_INIT_POINTER(sta->sdata->u.vlan.sta, NULL);

		if (test_sta_flag(sta, WLAN_STA_AUTHORIZED))
			ieee80211_vif_dec_num_mcast(sta->sdata);

		sta->sdata = vlansdata;
		ieee80211_check_fast_xmit(sta);

		if (test_sta_flag(sta, WLAN_STA_AUTHORIZED))
			ieee80211_vif_inc_num_mcast(sta->sdata);

		cfg80211_send_layer2_update(sta->sdata->dev, sta->sta.addr);
	}

	err = sta_apply_parameters(local, sta, params);
	if (err)
		goto out_err;

	mutex_unlock(&local->sta_mtx);

	if ((sdata->vif.type == NL80211_IFTYPE_AP ||
	     sdata->vif.type == NL80211_IFTYPE_AP_VLAN) &&
	    sta->known_smps_mode != sta->sdata->bss->req_smps &&
	    test_sta_flag(sta, WLAN_STA_AUTHORIZED) &&
	    sta_info_tx_streams(sta) != 1) {
		ht_dbg(sta->sdata,
		       "%pM just authorized and MIMO capable - update SMPS\n",
		       sta->sta.addr);
		ieee80211_send_smps_action(sta->sdata,
			sta->sdata->bss->req_smps,
			sta->sta.addr,
			sta->sdata->vif.bss_conf.bssid);
	}

	if (sdata->vif.type == NL80211_IFTYPE_STATION &&
	    params->sta_flags_mask & BIT(NL80211_STA_FLAG_AUTHORIZED)) {
		ieee80211_recalc_ps(local);
		ieee80211_recalc_ps_vif(sdata);
	}

	return 0;
out_err:
	mutex_unlock(&local->sta_mtx);
	return err;
}