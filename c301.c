struct udev_device *udev_monitor_receive_device(struct udev_monitor *udev_monitor)
{
	struct udev_device *udev_device;
	struct msghdr smsg;
	struct iovec iov;
	char cred_msg[CMSG_SPACE(sizeof(struct ucred))];
	char buf[4096];
	size_t bufpos;
	int devpath_set = 0;
	int subsystem_set = 0;
	int action_set = 0;
	int maj = 0;
	int min = 0;

	if (udev_monitor == NULL)
		return NULL;
	memset(buf, 0x00, sizeof(buf));
	iov.iov_base = &buf;
	iov.iov_len = sizeof(buf);
	memset (&smsg, 0x00, sizeof(struct msghdr));
	smsg.msg_iov = &iov;
	smsg.msg_iovlen = 1;
	smsg.msg_control = cred_msg;
	smsg.msg_controllen = sizeof(cred_msg);

	if (recvmsg(udev_monitor->sock, &smsg, 0) < 0) {
		if (errno != EINTR)
			info(udev_monitor->udev, "unable to receive message");
		return NULL;
	}

	if (udev_monitor->sun.sun_family != 0) {
		struct cmsghdr *cmsg = CMSG_FIRSTHDR(&smsg);
		struct ucred *cred = (struct ucred *)CMSG_DATA (cmsg);

		if (cmsg == NULL || cmsg->cmsg_type != SCM_CREDENTIALS) {
			info(udev_monitor->udev, "no sender credentials received, message ignored");
			return NULL;
		}

		if (cred->uid != 0) {
			info(udev_monitor->udev, "sender uid=%d, message ignored", cred->uid);
			return NULL;
		}
	}

	/* skip header */
	bufpos = strlen(buf) + 1;
	if (bufpos < sizeof("a@/d") || bufpos >= sizeof(buf)) {
		info(udev_monitor->udev, "invalid message length");
		return NULL;
	}

	/* check message header */
	if (strstr(buf, "@/") == NULL) {
		info(udev_monitor->udev, "unrecognized message header");
		return NULL;
	}

	udev_device = device_new(udev_monitor->udev);
	if (udev_device == NULL) {
		return NULL;
	}

	while (bufpos < sizeof(buf)) {
		char *key;
		size_t keylen;

		key = &buf[bufpos];
		keylen = strlen(key);
		if (keylen == 0)
			break;
		bufpos += keylen + 1;

		if (strncmp(key, "DEVPATH=", 8) == 0) {
			char path[UTIL_PATH_SIZE];

			util_strlcpy(path, udev_get_sys_path(udev_monitor->udev), sizeof(path));
			util_strlcat(path, &key[8], sizeof(path));
			udev_device_set_syspath(udev_device, path);
			devpath_set = 1;
		} else if (strncmp(key, "SUBSYSTEM=", 10) == 0) {
			udev_device_set_subsystem(udev_device, &key[10]);
			subsystem_set = 1;
		} else if (strncmp(key, "DEVTYPE=", 8) == 0) {
			udev_device_set_devtype(udev_device, &key[8]);
		} else if (strncmp(key, "DEVNAME=", 8) == 0) {
			udev_device_set_devnode(udev_device, &key[8]);
		} else if (strncmp(key, "DEVLINKS=", 9) == 0) {
			char devlinks[UTIL_PATH_SIZE];
			char *slink;
			char *next;

			util_strlcpy(devlinks, &key[9], sizeof(devlinks));
			slink = devlinks;
			next = strchr(slink, ' ');
			while (next != NULL) {
				next[0] = '\0';
				udev_device_add_devlink(udev_device, slink);
				slink = &next[1];
				next = strchr(slink, ' ');
			}
			if (slink[0] != '\0')
				udev_device_add_devlink(udev_device, slink);
		} else if (strncmp(key, "DRIVER=", 7) == 0) {
			udev_device_set_driver(udev_device, &key[7]);
		} else if (strncmp(key, "ACTION=", 7) == 0) {
			udev_device_set_action(udev_device, &key[7]);
			action_set = 1;
		} else if (strncmp(key, "MAJOR=", 6) == 0) {
			maj = strtoull(&key[6], NULL, 10);
		} else if (strncmp(key, "MINOR=", 6) == 0) {
			min = strtoull(&key[6], NULL, 10);
		} else if (strncmp(key, "DEVPATH_OLD=", 12) == 0) {
			udev_device_set_devpath_old(udev_device, &key[12]);
		} else if (strncmp(key, "PHYSDEVPATH=", 12) == 0) {
			udev_device_set_physdevpath(udev_device, &key[12]);
		} else if (strncmp(key, "SEQNUM=", 7) == 0) {
			udev_device_set_seqnum(udev_device, strtoull(&key[7], NULL, 10));
		} else if (strncmp(key, "TIMEOUT=", 8) == 0) {
			udev_device_set_timeout(udev_device, strtoull(&key[8], NULL, 10));
		} else if (strncmp(key, "PHYSDEV", 7) == 0) {
			/* skip deprecated values */
			continue;
		} else {
			udev_device_add_property_from_string(udev_device, key);
		}
	}
	if (!devpath_set || !subsystem_set || !action_set) {
		info(udev_monitor->udev, "missing values, skip\n");
		udev_device_unref(udev_device);
		return NULL;
	}
	if (maj > 0)
		udev_device_set_devnum(udev_device, makedev(maj, min));
	udev_device_set_info_loaded(udev_device);
	return udev_device;
}