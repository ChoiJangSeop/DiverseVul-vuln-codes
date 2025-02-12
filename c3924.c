dissect_pktap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_tree *pktap_tree = NULL;
	proto_item *ti = NULL;
	tvbuff_t *next_tvb;
	int offset = 0;
	guint32 pkt_len, rectype, dlt;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "PKTAP");
	col_clear(pinfo->cinfo, COL_INFO);

	pkt_len = tvb_get_letohl(tvb, offset);
	col_add_fstr(pinfo->cinfo, COL_INFO, "PKTAP, %u byte header", pkt_len);

	/* Dissect the packet */
	ti = proto_tree_add_item(tree, proto_pktap, tvb, offset, pkt_len, ENC_NA);
	pktap_tree = proto_item_add_subtree(ti, ett_pktap);

	proto_tree_add_item(pktap_tree, hf_pktap_hdrlen, tvb, offset, 4,
	    ENC_LITTLE_ENDIAN);
	if (pkt_len < MIN_PKTAP_HDR_LEN) {
		proto_tree_add_expert(tree, pinfo, &ei_pktap_hdrlen_too_short,
		    tvb, offset, 4);
		return;
	}
	offset += 4;

	proto_tree_add_item(pktap_tree, hf_pktap_rectype, tvb, offset, 4,
	    ENC_LITTLE_ENDIAN);
	rectype = tvb_get_letohl(tvb, offset);
	offset += 4;
	proto_tree_add_item(pktap_tree, hf_pktap_dlt, tvb, offset, 4,
	    ENC_LITTLE_ENDIAN);
	dlt = tvb_get_letohl(tvb, offset);
	offset += 4;
	proto_tree_add_item(pktap_tree, hf_pktap_ifname, tvb, offset, 24,
	    ENC_ASCII|ENC_NA);
	offset += 24;
	proto_tree_add_item(pktap_tree, hf_pktap_flags, tvb, offset, 4,
	    ENC_LITTLE_ENDIAN);
	offset += 4;
	proto_tree_add_item(pktap_tree, hf_pktap_pfamily, tvb, offset, 4,
	    ENC_LITTLE_ENDIAN);
	offset += 4;
	proto_tree_add_item(pktap_tree, hf_pktap_llhdrlen, tvb, offset, 4,
	    ENC_LITTLE_ENDIAN);
	offset += 4;
	proto_tree_add_item(pktap_tree, hf_pktap_lltrlrlen, tvb, offset, 4,
	    ENC_LITTLE_ENDIAN);
	offset += 4;
	proto_tree_add_item(pktap_tree, hf_pktap_pid, tvb, offset, 4,
	    ENC_LITTLE_ENDIAN);
	offset += 4;
	proto_tree_add_item(pktap_tree, hf_pktap_cmdname, tvb, offset, 20,
	    ENC_UTF_8|ENC_NA);
	offset += 20;
	proto_tree_add_item(pktap_tree, hf_pktap_svc_class, tvb, offset, 4,
	    ENC_LITTLE_ENDIAN);
	offset += 4;
	proto_tree_add_item(pktap_tree, hf_pktap_iftype, tvb, offset, 2,
	    ENC_LITTLE_ENDIAN);
	offset += 2;
	proto_tree_add_item(pktap_tree, hf_pktap_ifunit, tvb, offset, 2,
	    ENC_LITTLE_ENDIAN);
	offset += 2;
	proto_tree_add_item(pktap_tree, hf_pktap_epid, tvb, offset, 4,
	    ENC_LITTLE_ENDIAN);
	offset += 4;
	proto_tree_add_item(pktap_tree, hf_pktap_ecmdname, tvb, offset, 20,
	    ENC_UTF_8|ENC_NA);
	/*offset += 20;*/

	if (rectype == PKT_REC_PACKET) {
		next_tvb = tvb_new_subset_remaining(tvb, pkt_len);
		dissector_try_uint(wtap_encap_dissector_table,
		    wtap_pcap_encap_to_wtap_encap(dlt), next_tvb, pinfo, tree);
	}
}