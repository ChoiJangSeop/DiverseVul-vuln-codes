static gboolean avdtp_setconf_cmd(struct avdtp *session, uint8_t transaction,
				struct setconf_req *req, unsigned int size)
{
	struct conf_rej rej;
	struct avdtp_local_sep *sep;
	struct avdtp_stream *stream;
	uint8_t err, category = 0x00;
	struct btd_service *service;
	GSList *l;

	if (size < sizeof(struct setconf_req)) {
		error("Too short getcap request");
		return FALSE;
	}

	sep = find_local_sep_by_seid(session, req->acp_seid);
	if (!sep) {
		err = AVDTP_BAD_ACP_SEID;
		goto failed;
	}

	if (sep->stream) {
		err = AVDTP_SEP_IN_USE;
		goto failed;
	}

	switch (sep->info.type) {
	case AVDTP_SEP_TYPE_SOURCE:
		service = btd_device_get_service(session->device,
							A2DP_SINK_UUID);
		if (service == NULL) {
			btd_device_add_uuid(session->device, A2DP_SINK_UUID);
			service = btd_device_get_service(session->device,
							A2DP_SINK_UUID);
			if (service == NULL) {
				error("Unable to get a audio sink object");
				err = AVDTP_BAD_STATE;
				goto failed;
			}
		}
		break;
	case AVDTP_SEP_TYPE_SINK:
		service = btd_device_get_service(session->device,
							A2DP_SOURCE_UUID);
		if (service == NULL) {
			btd_device_add_uuid(session->device, A2DP_SOURCE_UUID);
			service = btd_device_get_service(session->device,
							A2DP_SOURCE_UUID);
			if (service == NULL) {
				error("Unable to get a audio source object");
				err = AVDTP_BAD_STATE;
				goto failed;
			}
		}
		break;
	}

	stream = g_new0(struct avdtp_stream, 1);
	stream->session = session;
	stream->lsep = sep;
	stream->rseid = req->int_seid;
	stream->caps = caps_to_list(req->caps,
					size - sizeof(struct setconf_req),
					&stream->codec,
					&stream->delay_reporting);

	/* Verify that the Media Transport capability's length = 0. Reject otherwise */
	for (l = stream->caps; l != NULL; l = g_slist_next(l)) {
		struct avdtp_service_capability *cap = l->data;

		if (cap->category == AVDTP_MEDIA_TRANSPORT && cap->length != 0) {
			err = AVDTP_BAD_MEDIA_TRANSPORT_FORMAT;
			goto failed_stream;
		}
	}

	if (stream->delay_reporting && session->version < 0x0103)
		session->version = 0x0103;

	if (sep->ind && sep->ind->set_configuration) {
		if (!sep->ind->set_configuration(session, sep, stream,
							stream->caps,
							setconf_cb,
							sep->user_data)) {
			err = AVDTP_UNSUPPORTED_CONFIGURATION;
			category = 0x00;
			goto failed_stream;
		}
	} else {
		if (!avdtp_send(session, transaction, AVDTP_MSG_TYPE_ACCEPT,
					AVDTP_SET_CONFIGURATION, NULL, 0)) {
			stream_free(stream);
			return FALSE;
		}

		sep->stream = stream;
		sep->info.inuse = 1;
		session->streams = g_slist_append(session->streams, stream);

		avdtp_sep_set_state(session, sep, AVDTP_STATE_CONFIGURED);
	}

	return TRUE;

failed_stream:
	stream_free(stream);
failed:
	rej.error = err;
	rej.category = category;
	return avdtp_send(session, transaction, AVDTP_MSG_TYPE_REJECT,
				AVDTP_SET_CONFIGURATION, &rej, sizeof(rej));
}