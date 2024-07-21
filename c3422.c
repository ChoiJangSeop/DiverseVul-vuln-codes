apply_enable_disable(
	attr_val_fifo *	fifo,
	int		enable
	)
{
	attr_val *curr_flag;
	int option;
#ifdef BC_LIST_FRAMEWORK_NOT_YET_USED
	bc_entry *pentry;
#endif

	for (curr_flag = HEAD_PFIFO(fifo);
	     curr_flag != NULL;
	     curr_flag = curr_flag->link) {

		option = curr_flag->value.i;
		switch (option) {

		default:
			msyslog(LOG_ERR,
				"can not apply enable/disable token %d, unknown",
				option);
			break;

		case T_Auth:
			proto_config(PROTO_AUTHENTICATE, enable, 0., NULL);
			break;

		case T_Bclient:
			proto_config(PROTO_BROADCLIENT, enable, 0., NULL);
			break;

		case T_Calibrate:
			proto_config(PROTO_CAL, enable, 0., NULL);
			break;

		case T_Kernel:
			proto_config(PROTO_KERNEL, enable, 0., NULL);
			break;

		case T_Monitor:
			proto_config(PROTO_MONITOR, enable, 0., NULL);
			break;

		case T_Ntp:
			proto_config(PROTO_NTP, enable, 0., NULL);
			break;

		case T_Mode7:
			proto_config(PROTO_MODE7, enable, 0., NULL);
			break;

		case T_Stats:
			proto_config(PROTO_FILEGEN, enable, 0., NULL);
			break;

#ifdef BC_LIST_FRAMEWORK_NOT_YET_USED
		case T_Bc_bugXXXX:
			pentry = bc_list;
			while (pentry->token) {
				if (pentry->token == option)
					break;
				pentry++;
			}
			if (!pentry->token) {
				msyslog(LOG_ERR,
					"compat token %d not in bc_list[]",
					option);
				continue;
			}
			pentry->enabled = enable;
			break;
#endif
		}
	}
}