
static int nl80211_start_sched_scan(struct sk_buff *skb,
				    struct genl_info *info)
{
	struct cfg80211_sched_scan_request *request;
	struct cfg80211_registered_device *rdev = info->user_ptr[0];
	struct net_device *dev = info->user_ptr[1];
	struct nlattr *attr;
	struct wiphy *wiphy;
	int err, tmp, n_ssids = 0, n_channels, i;
	u32 interval;
	enum ieee80211_band band;
	size_t ie_len;

	if (!(rdev->wiphy.flags & WIPHY_FLAG_SUPPORTS_SCHED_SCAN) ||
	    !rdev->ops->sched_scan_start)
		return -EOPNOTSUPP;

	if (!is_valid_ie_attr(info->attrs[NL80211_ATTR_IE]))
		return -EINVAL;

	if (rdev->sched_scan_req)
		return -EINPROGRESS;

	if (!info->attrs[NL80211_ATTR_SCHED_SCAN_INTERVAL])
		return -EINVAL;

	interval = nla_get_u32(info->attrs[NL80211_ATTR_SCHED_SCAN_INTERVAL]);
	if (interval == 0)
		return -EINVAL;

	wiphy = &rdev->wiphy;

	if (info->attrs[NL80211_ATTR_SCAN_FREQUENCIES]) {
		n_channels = validate_scan_freqs(
				info->attrs[NL80211_ATTR_SCAN_FREQUENCIES]);
		if (!n_channels)
			return -EINVAL;
	} else {
		n_channels = 0;

		for (band = 0; band < IEEE80211_NUM_BANDS; band++)
			if (wiphy->bands[band])
				n_channels += wiphy->bands[band]->n_channels;
	}

	if (info->attrs[NL80211_ATTR_SCAN_SSIDS])
		nla_for_each_nested(attr, info->attrs[NL80211_ATTR_SCAN_SSIDS],
				    tmp)
			n_ssids++;

	if (n_ssids > wiphy->max_scan_ssids)
		return -EINVAL;

	if (info->attrs[NL80211_ATTR_IE])
		ie_len = nla_len(info->attrs[NL80211_ATTR_IE]);
	else
		ie_len = 0;

	if (ie_len > wiphy->max_scan_ie_len)
		return -EINVAL;

	request = kzalloc(sizeof(*request)
			+ sizeof(*request->ssids) * n_ssids
			+ sizeof(*request->channels) * n_channels
			+ ie_len, GFP_KERNEL);
	if (!request)
		return -ENOMEM;

	if (n_ssids)
		request->ssids = (void *)&request->channels[n_channels];
	request->n_ssids = n_ssids;
	if (ie_len) {
		if (request->ssids)
			request->ie = (void *)(request->ssids + n_ssids);
		else
			request->ie = (void *)(request->channels + n_channels);
	}

	i = 0;
	if (info->attrs[NL80211_ATTR_SCAN_FREQUENCIES]) {
		/* user specified, bail out if channel not found */
		nla_for_each_nested(attr,
				    info->attrs[NL80211_ATTR_SCAN_FREQUENCIES],
				    tmp) {
			struct ieee80211_channel *chan;

			chan = ieee80211_get_channel(wiphy, nla_get_u32(attr));

			if (!chan) {
				err = -EINVAL;
				goto out_free;
			}

			/* ignore disabled channels */
			if (chan->flags & IEEE80211_CHAN_DISABLED)
				continue;

			request->channels[i] = chan;
			i++;
		}
	} else {
		/* all channels */
		for (band = 0; band < IEEE80211_NUM_BANDS; band++) {
			int j;
			if (!wiphy->bands[band])
				continue;
			for (j = 0; j < wiphy->bands[band]->n_channels; j++) {
				struct ieee80211_channel *chan;

				chan = &wiphy->bands[band]->channels[j];

				if (chan->flags & IEEE80211_CHAN_DISABLED)
					continue;

				request->channels[i] = chan;
				i++;
			}
		}
	}

	if (!i) {
		err = -EINVAL;
		goto out_free;
	}

	request->n_channels = i;

	i = 0;
	if (info->attrs[NL80211_ATTR_SCAN_SSIDS]) {
		nla_for_each_nested(attr, info->attrs[NL80211_ATTR_SCAN_SSIDS],
				    tmp) {
			if (request->ssids[i].ssid_len >
			    IEEE80211_MAX_SSID_LEN) {
				err = -EINVAL;
				goto out_free;
			}
			memcpy(request->ssids[i].ssid, nla_data(attr),
			       nla_len(attr));
			request->ssids[i].ssid_len = nla_len(attr);
			i++;
		}
	}

	if (info->attrs[NL80211_ATTR_IE]) {
		request->ie_len = nla_len(info->attrs[NL80211_ATTR_IE]);
		memcpy((void *)request->ie,
		       nla_data(info->attrs[NL80211_ATTR_IE]),
		       request->ie_len);
	}

	request->dev = dev;
	request->wiphy = &rdev->wiphy;
	request->interval = interval;

	err = rdev->ops->sched_scan_start(&rdev->wiphy, dev, request);
	if (!err) {
		rdev->sched_scan_req = request;
		nl80211_send_sched_scan(rdev, dev,
					NL80211_CMD_START_SCHED_SCAN);
		goto out;
	}

out_free:
	kfree(request);
out:
	return err;