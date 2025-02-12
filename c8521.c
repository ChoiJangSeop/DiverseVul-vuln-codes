static int ieee80211_add_station(struct wiphy *wiphy, struct net_device *dev,
				 const u8 *mac,
				 struct station_parameters *params)
{
	struct ieee80211_local *local = wiphy_priv(wiphy);
	struct sta_info *sta;
	struct ieee80211_sub_if_data *sdata;
	int err;
	int layer2_update;

	if (params->vlan) {
		sdata = IEEE80211_DEV_TO_SUB_IF(params->vlan);

		if (sdata->vif.type != NL80211_IFTYPE_AP_VLAN &&
		    sdata->vif.type != NL80211_IFTYPE_AP)
			return -EINVAL;
	} else
		sdata = IEEE80211_DEV_TO_SUB_IF(dev);

	if (ether_addr_equal(mac, sdata->vif.addr))
		return -EINVAL;

	if (is_multicast_ether_addr(mac))
		return -EINVAL;

	if (params->sta_flags_set & BIT(NL80211_STA_FLAG_TDLS_PEER) &&
	    sdata->vif.type == NL80211_IFTYPE_STATION &&
	    !sdata->u.mgd.associated)
		return -EINVAL;

	sta = sta_info_alloc(sdata, mac, GFP_KERNEL);
	if (!sta)
		return -ENOMEM;

	if (params->sta_flags_set & BIT(NL80211_STA_FLAG_TDLS_PEER))
		sta->sta.tdls = true;

	err = sta_apply_parameters(local, sta, params);
	if (err) {
		sta_info_free(local, sta);
		return err;
	}

	/*
	 * for TDLS and for unassociated station, rate control should be
	 * initialized only when rates are known and station is marked
	 * authorized/associated
	 */
	if (!test_sta_flag(sta, WLAN_STA_TDLS_PEER) &&
	    test_sta_flag(sta, WLAN_STA_ASSOC))
		rate_control_rate_init(sta);

	layer2_update = sdata->vif.type == NL80211_IFTYPE_AP_VLAN ||
		sdata->vif.type == NL80211_IFTYPE_AP;

	err = sta_info_insert_rcu(sta);
	if (err) {
		rcu_read_unlock();
		return err;
	}

	if (layer2_update)
		cfg80211_send_layer2_update(sta->sdata->dev, sta->sta.addr);

	rcu_read_unlock();

	return 0;
}