GF_Err gf_odf_read_descriptor(GF_BitStream *bs, GF_Descriptor *desc, u32 DescSize)
{
	switch (desc->tag) {
	case GF_ODF_IOD_TAG :
		return gf_odf_read_iod(bs, (GF_InitialObjectDescriptor *)desc, DescSize);
	case GF_ODF_ESD_TAG :
		return gf_odf_read_esd(bs, (GF_ESD *)desc, DescSize);
	case GF_ODF_DCD_TAG :
		return gf_odf_read_dcd(bs, (GF_DecoderConfig *)desc, DescSize);
	case GF_ODF_SLC_TAG :
		return gf_odf_read_slc(bs, (GF_SLConfig *)desc, DescSize);
	case GF_ODF_OD_TAG:
		return gf_odf_read_od(bs, (GF_ObjectDescriptor *)desc, DescSize);

	//MP4 File Format
	case GF_ODF_ISOM_IOD_TAG:
		return gf_odf_read_isom_iod(bs, (GF_IsomInitialObjectDescriptor *)desc, DescSize);
	case GF_ODF_ISOM_OD_TAG:
		return gf_odf_read_isom_od(bs, (GF_IsomObjectDescriptor *)desc, DescSize);
	case GF_ODF_ESD_INC_TAG:
		return gf_odf_read_esd_inc(bs, (GF_ES_ID_Inc *)desc, DescSize);
	case GF_ODF_ESD_REF_TAG:
		return gf_odf_read_esd_ref(bs, (GF_ES_ID_Ref *)desc, DescSize);

	case GF_ODF_SEGMENT_TAG:
		return gf_odf_read_segment(bs, (GF_Segment *) desc, DescSize);
	case GF_ODF_MUXINFO_TAG:
		GF_LOG(GF_LOG_ERROR, GF_LOG_CODING, ("[ODF] MuxInfo descriptor cannot be read, wrong serialization or conflict with other user-space OD tags\n"));
		return GF_NON_COMPLIANT_BITSTREAM;

	case GF_ODF_AUX_VIDEO_DATA:
		return gf_odf_read_auxvid(bs, (GF_AuxVideoDescriptor *)desc, DescSize);

	case GF_ODF_LANG_TAG:
	case GF_ODF_GPAC_LANG:
		return gf_odf_read_lang(bs, (GF_Language *)desc, DescSize);

#ifndef GPAC_MINIMAL_ODF
	case GF_ODF_MEDIATIME_TAG:
		return gf_odf_read_mediatime(bs, (GF_MediaTime *) desc, DescSize);
	case GF_ODF_IPMP_TAG:
		return gf_odf_read_ipmp(bs, (GF_IPMP_Descriptor *)desc, DescSize);
	case GF_ODF_IPMP_PTR_TAG:
		return gf_odf_read_ipmp_ptr(bs, (GF_IPMPPtr *)desc, DescSize);

	case GF_ODF_CC_TAG:
		return gf_odf_read_cc(bs, (GF_CCDescriptor *)desc, DescSize);
	case GF_ODF_CC_DATE_TAG:
		return gf_odf_read_cc_date(bs, (GF_CC_Date *)desc, DescSize);
	case GF_ODF_CC_NAME_TAG:
		return gf_odf_read_cc_name(bs, (GF_CC_Name *)desc, DescSize);
	case GF_ODF_CI_TAG:
		return gf_odf_read_ci(bs, (GF_CIDesc *)desc, DescSize);
	case GF_ODF_TEXT_TAG:
		return gf_odf_read_exp_text(bs, (GF_ExpandedTextual *)desc, DescSize);
	case GF_ODF_EXT_PL_TAG:
		return gf_odf_read_pl_ext(bs, (GF_PLExt *)desc, DescSize);
	case GF_ODF_IPI_PTR_TAG:
	case GF_ODF_ISOM_IPI_PTR_TAG:
		return gf_odf_read_ipi_ptr(bs, (GF_IPIPtr *)desc, DescSize);
	case GF_ODF_KW_TAG:
		return gf_odf_read_kw(bs, (GF_KeyWord *)desc, DescSize);
	case GF_ODF_OCI_DATE_TAG:
		return gf_odf_read_oci_date(bs, (GF_OCI_Data *)desc, DescSize);
	case GF_ODF_OCI_NAME_TAG:
		return gf_odf_read_oci_name(bs, (GF_OCICreators *)desc, DescSize);
	case GF_ODF_PL_IDX_TAG:
		return gf_odf_read_pl_idx(bs, (GF_PL_IDX *)desc, DescSize);
	case GF_ODF_QOS_TAG:
		return gf_odf_read_qos(bs, (GF_QoS_Descriptor *)desc, DescSize);
	case GF_ODF_RATING_TAG:
		return gf_odf_read_rating(bs, (GF_Rating *)desc, DescSize);
	case GF_ODF_REG_TAG:
		return gf_odf_read_reg(bs, (GF_Registration *)desc, DescSize);
	case GF_ODF_SHORT_TEXT_TAG:
		return gf_odf_read_short_text(bs, (GF_ShortTextual *)desc, DescSize);
	case GF_ODF_SMPTE_TAG:
		return gf_odf_read_smpte_camera(bs, (GF_SMPTECamera *)desc, DescSize);
	case GF_ODF_SCI_TAG:
		return gf_odf_read_sup_cid(bs, (GF_SCIDesc *)desc, DescSize);

	case GF_ODF_IPMP_TL_TAG:
		return gf_odf_read_ipmp_tool_list(bs, (GF_IPMP_ToolList *)desc, DescSize);
	case GF_ODF_IPMP_TOOL_TAG:
		return gf_odf_read_ipmp_tool(bs, (GF_IPMP_Tool *)desc, DescSize);

#endif /*GPAC_MINIMAL_ODF*/
	//default:
	case GF_ODF_DSI_TAG:
	default:
		return gf_odf_read_default(bs, (GF_DefaultDescriptor *)desc, DescSize);
	}
	return GF_OK;
}