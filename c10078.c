int iscsi_conn_get_param(struct iscsi_cls_conn *cls_conn,
			 enum iscsi_param param, char *buf)
{
	struct iscsi_conn *conn = cls_conn->dd_data;
	int len;

	switch(param) {
	case ISCSI_PARAM_PING_TMO:
		len = sprintf(buf, "%u\n", conn->ping_timeout);
		break;
	case ISCSI_PARAM_RECV_TMO:
		len = sprintf(buf, "%u\n", conn->recv_timeout);
		break;
	case ISCSI_PARAM_MAX_RECV_DLENGTH:
		len = sprintf(buf, "%u\n", conn->max_recv_dlength);
		break;
	case ISCSI_PARAM_MAX_XMIT_DLENGTH:
		len = sprintf(buf, "%u\n", conn->max_xmit_dlength);
		break;
	case ISCSI_PARAM_HDRDGST_EN:
		len = sprintf(buf, "%d\n", conn->hdrdgst_en);
		break;
	case ISCSI_PARAM_DATADGST_EN:
		len = sprintf(buf, "%d\n", conn->datadgst_en);
		break;
	case ISCSI_PARAM_IFMARKER_EN:
		len = sprintf(buf, "%d\n", conn->ifmarker_en);
		break;
	case ISCSI_PARAM_OFMARKER_EN:
		len = sprintf(buf, "%d\n", conn->ofmarker_en);
		break;
	case ISCSI_PARAM_EXP_STATSN:
		len = sprintf(buf, "%u\n", conn->exp_statsn);
		break;
	case ISCSI_PARAM_PERSISTENT_PORT:
		len = sprintf(buf, "%d\n", conn->persistent_port);
		break;
	case ISCSI_PARAM_PERSISTENT_ADDRESS:
		len = sprintf(buf, "%s\n", conn->persistent_address);
		break;
	case ISCSI_PARAM_STATSN:
		len = sprintf(buf, "%u\n", conn->statsn);
		break;
	case ISCSI_PARAM_MAX_SEGMENT_SIZE:
		len = sprintf(buf, "%u\n", conn->max_segment_size);
		break;
	case ISCSI_PARAM_KEEPALIVE_TMO:
		len = sprintf(buf, "%u\n", conn->keepalive_tmo);
		break;
	case ISCSI_PARAM_LOCAL_PORT:
		len = sprintf(buf, "%u\n", conn->local_port);
		break;
	case ISCSI_PARAM_TCP_TIMESTAMP_STAT:
		len = sprintf(buf, "%u\n", conn->tcp_timestamp_stat);
		break;
	case ISCSI_PARAM_TCP_NAGLE_DISABLE:
		len = sprintf(buf, "%u\n", conn->tcp_nagle_disable);
		break;
	case ISCSI_PARAM_TCP_WSF_DISABLE:
		len = sprintf(buf, "%u\n", conn->tcp_wsf_disable);
		break;
	case ISCSI_PARAM_TCP_TIMER_SCALE:
		len = sprintf(buf, "%u\n", conn->tcp_timer_scale);
		break;
	case ISCSI_PARAM_TCP_TIMESTAMP_EN:
		len = sprintf(buf, "%u\n", conn->tcp_timestamp_en);
		break;
	case ISCSI_PARAM_IP_FRAGMENT_DISABLE:
		len = sprintf(buf, "%u\n", conn->fragment_disable);
		break;
	case ISCSI_PARAM_IPV4_TOS:
		len = sprintf(buf, "%u\n", conn->ipv4_tos);
		break;
	case ISCSI_PARAM_IPV6_TC:
		len = sprintf(buf, "%u\n", conn->ipv6_traffic_class);
		break;
	case ISCSI_PARAM_IPV6_FLOW_LABEL:
		len = sprintf(buf, "%u\n", conn->ipv6_flow_label);
		break;
	case ISCSI_PARAM_IS_FW_ASSIGNED_IPV6:
		len = sprintf(buf, "%u\n", conn->is_fw_assigned_ipv6);
		break;
	case ISCSI_PARAM_TCP_XMIT_WSF:
		len = sprintf(buf, "%u\n", conn->tcp_xmit_wsf);
		break;
	case ISCSI_PARAM_TCP_RECV_WSF:
		len = sprintf(buf, "%u\n", conn->tcp_recv_wsf);
		break;
	case ISCSI_PARAM_LOCAL_IPADDR:
		len = sprintf(buf, "%s\n", conn->local_ipaddr);
		break;
	default:
		return -ENOSYS;
	}

	return len;
}