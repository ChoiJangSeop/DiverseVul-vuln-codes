static int uvc_scan_chain_backward(struct uvc_video_chain *chain,
	struct uvc_entity **_entity)
{
	struct uvc_entity *entity = *_entity;
	struct uvc_entity *term;
	int id = -EINVAL, i;

	switch (UVC_ENTITY_TYPE(entity)) {
	case UVC_VC_EXTENSION_UNIT:
	case UVC_VC_PROCESSING_UNIT:
		id = entity->baSourceID[0];
		break;

	case UVC_VC_SELECTOR_UNIT:
		/* Single-input selector units are ignored. */
		if (entity->bNrInPins == 1) {
			id = entity->baSourceID[0];
			break;
		}

		if (uvc_trace_param & UVC_TRACE_PROBE)
			printk(KERN_CONT " <- IT");

		chain->selector = entity;
		for (i = 0; i < entity->bNrInPins; ++i) {
			id = entity->baSourceID[i];
			term = uvc_entity_by_id(chain->dev, id);
			if (term == NULL || !UVC_ENTITY_IS_ITERM(term)) {
				uvc_trace(UVC_TRACE_DESCR, "Selector unit %d "
					"input %d isn't connected to an "
					"input terminal\n", entity->id, i);
				return -1;
			}

			if (uvc_trace_param & UVC_TRACE_PROBE)
				printk(KERN_CONT " %d", term->id);

			list_add_tail(&term->chain, &chain->entities);
			uvc_scan_chain_forward(chain, term, entity);
		}

		if (uvc_trace_param & UVC_TRACE_PROBE)
			printk(KERN_CONT "\n");

		id = 0;
		break;

	case UVC_ITT_VENDOR_SPECIFIC:
	case UVC_ITT_CAMERA:
	case UVC_ITT_MEDIA_TRANSPORT_INPUT:
	case UVC_OTT_VENDOR_SPECIFIC:
	case UVC_OTT_DISPLAY:
	case UVC_OTT_MEDIA_TRANSPORT_OUTPUT:
	case UVC_TT_STREAMING:
		id = UVC_ENTITY_IS_OTERM(entity) ? entity->baSourceID[0] : 0;
		break;
	}

	if (id <= 0) {
		*_entity = NULL;
		return id;
	}

	entity = uvc_entity_by_id(chain->dev, id);
	if (entity == NULL) {
		uvc_trace(UVC_TRACE_DESCR, "Found reference to "
			"unknown entity %d.\n", id);
		return -EINVAL;
	}

	*_entity = entity;
	return 0;
}