static int hog_accept(struct btd_service *service)
{
	struct hog_device *dev = btd_service_get_user_data(service);
	struct btd_device *device = btd_service_get_device(service);
	struct gatt_db *db = btd_device_get_gatt_db(device);
	GAttrib *attrib = btd_device_get_attrib(device);

	if (!dev->hog) {
		hog_device_accept(dev, db);
		if (!dev->hog)
			return -EINVAL;
	}

	/* TODO: Replace GAttrib with bt_gatt_client */
	bt_hog_attach(dev->hog, attrib);

	btd_service_connecting_complete(service, 0);

	return 0;
}