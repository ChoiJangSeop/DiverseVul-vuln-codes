void pjsua_init_tpselector(pjsua_transport_id tp_id,
			   pjsip_tpselector *sel)
{
    pjsua_transport_data *tpdata;
    unsigned flag;

    pj_bzero(sel, sizeof(*sel));
    if (tp_id == PJSUA_INVALID_ID)
	return;

    pj_assert(tp_id >= 0 && tp_id < (int)PJ_ARRAY_SIZE(pjsua_var.tpdata));
    tpdata = &pjsua_var.tpdata[tp_id];

    flag = pjsip_transport_get_flag_from_type(tpdata->type);

    if (flag & PJSIP_TRANSPORT_DATAGRAM) {
	sel->type = PJSIP_TPSELECTOR_TRANSPORT;
	sel->u.transport = tpdata->data.tp;
    } else {
	sel->type = PJSIP_TPSELECTOR_LISTENER;
	sel->u.listener = tpdata->data.factory;
    }
}