static void ieee80211_rx_mgmt_deauth(struct ieee80211_sub_if_data *sdata,
				     struct ieee80211_mgmt *mgmt, size_t len)
{
	struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
	u16 reason_code = le16_to_cpu(mgmt->u.deauth.reason_code);

	sdata_assert_lock(sdata);

	if (len < 24 + 2)
		return;

	if (ifmgd->associated &&
	    ether_addr_equal(mgmt->bssid, ifmgd->associated->bssid)) {
		const u8 *bssid = ifmgd->associated->bssid;

		sdata_info(sdata, "deauthenticated from %pM (Reason: %u=%s)\n",
			   bssid, reason_code,
			   ieee80211_get_reason_code_string(reason_code));

		ieee80211_set_disassoc(sdata, 0, 0, false, NULL);

		ieee80211_report_disconnect(sdata, (u8 *)mgmt, len, false,
					    reason_code);
		return;
	}

	if (ifmgd->assoc_data &&
	    ether_addr_equal(mgmt->bssid, ifmgd->assoc_data->bss->bssid)) {
		const u8 *bssid = ifmgd->assoc_data->bss->bssid;

		sdata_info(sdata,
			   "deauthenticated from %pM while associating (Reason: %u=%s)\n",
			   bssid, reason_code,
			   ieee80211_get_reason_code_string(reason_code));

		ieee80211_destroy_assoc_data(sdata, false, true);

		cfg80211_rx_mlme_mgmt(sdata->dev, (u8 *)mgmt, len);
		return;
	}
}