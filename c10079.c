int iscsi_session_get_param(struct iscsi_cls_session *cls_session,
			    enum iscsi_param param, char *buf)
{
	struct iscsi_session *session = cls_session->dd_data;
	int len;

	switch(param) {
	case ISCSI_PARAM_FAST_ABORT:
		len = sprintf(buf, "%d\n", session->fast_abort);
		break;
	case ISCSI_PARAM_ABORT_TMO:
		len = sprintf(buf, "%d\n", session->abort_timeout);
		break;
	case ISCSI_PARAM_LU_RESET_TMO:
		len = sprintf(buf, "%d\n", session->lu_reset_timeout);
		break;
	case ISCSI_PARAM_TGT_RESET_TMO:
		len = sprintf(buf, "%d\n", session->tgt_reset_timeout);
		break;
	case ISCSI_PARAM_INITIAL_R2T_EN:
		len = sprintf(buf, "%d\n", session->initial_r2t_en);
		break;
	case ISCSI_PARAM_MAX_R2T:
		len = sprintf(buf, "%hu\n", session->max_r2t);
		break;
	case ISCSI_PARAM_IMM_DATA_EN:
		len = sprintf(buf, "%d\n", session->imm_data_en);
		break;
	case ISCSI_PARAM_FIRST_BURST:
		len = sprintf(buf, "%u\n", session->first_burst);
		break;
	case ISCSI_PARAM_MAX_BURST:
		len = sprintf(buf, "%u\n", session->max_burst);
		break;
	case ISCSI_PARAM_PDU_INORDER_EN:
		len = sprintf(buf, "%d\n", session->pdu_inorder_en);
		break;
	case ISCSI_PARAM_DATASEQ_INORDER_EN:
		len = sprintf(buf, "%d\n", session->dataseq_inorder_en);
		break;
	case ISCSI_PARAM_DEF_TASKMGMT_TMO:
		len = sprintf(buf, "%d\n", session->def_taskmgmt_tmo);
		break;
	case ISCSI_PARAM_ERL:
		len = sprintf(buf, "%d\n", session->erl);
		break;
	case ISCSI_PARAM_TARGET_NAME:
		len = sprintf(buf, "%s\n", session->targetname);
		break;
	case ISCSI_PARAM_TARGET_ALIAS:
		len = sprintf(buf, "%s\n", session->targetalias);
		break;
	case ISCSI_PARAM_TPGT:
		len = sprintf(buf, "%d\n", session->tpgt);
		break;
	case ISCSI_PARAM_USERNAME:
		len = sprintf(buf, "%s\n", session->username);
		break;
	case ISCSI_PARAM_USERNAME_IN:
		len = sprintf(buf, "%s\n", session->username_in);
		break;
	case ISCSI_PARAM_PASSWORD:
		len = sprintf(buf, "%s\n", session->password);
		break;
	case ISCSI_PARAM_PASSWORD_IN:
		len = sprintf(buf, "%s\n", session->password_in);
		break;
	case ISCSI_PARAM_IFACE_NAME:
		len = sprintf(buf, "%s\n", session->ifacename);
		break;
	case ISCSI_PARAM_INITIATOR_NAME:
		len = sprintf(buf, "%s\n", session->initiatorname);
		break;
	case ISCSI_PARAM_BOOT_ROOT:
		len = sprintf(buf, "%s\n", session->boot_root);
		break;
	case ISCSI_PARAM_BOOT_NIC:
		len = sprintf(buf, "%s\n", session->boot_nic);
		break;
	case ISCSI_PARAM_BOOT_TARGET:
		len = sprintf(buf, "%s\n", session->boot_target);
		break;
	case ISCSI_PARAM_AUTO_SND_TGT_DISABLE:
		len = sprintf(buf, "%u\n", session->auto_snd_tgt_disable);
		break;
	case ISCSI_PARAM_DISCOVERY_SESS:
		len = sprintf(buf, "%u\n", session->discovery_sess);
		break;
	case ISCSI_PARAM_PORTAL_TYPE:
		len = sprintf(buf, "%s\n", session->portal_type);
		break;
	case ISCSI_PARAM_CHAP_AUTH_EN:
		len = sprintf(buf, "%u\n", session->chap_auth_en);
		break;
	case ISCSI_PARAM_DISCOVERY_LOGOUT_EN:
		len = sprintf(buf, "%u\n", session->discovery_logout_en);
		break;
	case ISCSI_PARAM_BIDI_CHAP_EN:
		len = sprintf(buf, "%u\n", session->bidi_chap_en);
		break;
	case ISCSI_PARAM_DISCOVERY_AUTH_OPTIONAL:
		len = sprintf(buf, "%u\n", session->discovery_auth_optional);
		break;
	case ISCSI_PARAM_DEF_TIME2WAIT:
		len = sprintf(buf, "%d\n", session->time2wait);
		break;
	case ISCSI_PARAM_DEF_TIME2RETAIN:
		len = sprintf(buf, "%d\n", session->time2retain);
		break;
	case ISCSI_PARAM_TSID:
		len = sprintf(buf, "%u\n", session->tsid);
		break;
	case ISCSI_PARAM_ISID:
		len = sprintf(buf, "%02x%02x%02x%02x%02x%02x\n",
			      session->isid[0], session->isid[1],
			      session->isid[2], session->isid[3],
			      session->isid[4], session->isid[5]);
		break;
	case ISCSI_PARAM_DISCOVERY_PARENT_IDX:
		len = sprintf(buf, "%u\n", session->discovery_parent_idx);
		break;
	case ISCSI_PARAM_DISCOVERY_PARENT_TYPE:
		if (session->discovery_parent_type)
			len = sprintf(buf, "%s\n",
				      session->discovery_parent_type);
		else
			len = sprintf(buf, "\n");
		break;
	default:
		return -ENOSYS;
	}

	return len;
}