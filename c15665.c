void cms_set_pw_data(cms_context *cms, secuPWData *pwdata)
{
	ingress();

	switch (cms->pwdata.source) {
	case PW_SOURCE_INVALID:
	case PW_PROMPT:
	case PW_DEVICE:
	case PW_SOURCE_MAX:
		break;

	case PW_FROMFD:
		if (cms->pwdata.intdata >= 0 &&
		    !(pwdata->source == PW_FROMFD &&
		      cms->pwdata.intdata == pwdata->intdata))
			close(cms->pwdata.intdata);
		break;

	case PW_FROMFILEDB:
	case PW_FROMENV:
	case PW_FROMFILE:
	case PW_PLAINTEXT:
		memset(cms->pwdata.data, 0, strlen(cms->pwdata.data));
		xfree(cms->pwdata.data);
		break;

	case PW_DATABASE:
		xfree(cms->pwdata.data);
		break;
	}
	memmove(&cms->pwdata, pwdata, sizeof(*pwdata));

	dprintf("pwdata:%p", pwdata);
	dprintf("pwdata->source:%d", pwdata->source);
	dprintf("pwdata->data:%p (\"%s\")", pwdata->data,
		pwdata->data ? pwdata->data : "(null)");
	egress();
}