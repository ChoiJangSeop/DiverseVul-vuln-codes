GF_Descriptor *gf_isom_get_root_od(GF_ISOFile *movie)
{
	GF_Descriptor *desc;
	GF_ObjectDescriptor *od;
	GF_InitialObjectDescriptor *iod;
	GF_IsomObjectDescriptor *isom_od;
	GF_IsomInitialObjectDescriptor *isom_iod;
	GF_ESD *esd;
	GF_ES_ID_Inc *inc;
	u32 i;
	u8 useIOD;

	if (!movie || !movie->moov) return NULL;
	if (!movie->moov->iods) return NULL;

	if (movie->disable_odf_translate) {
		//duplicate our descriptor
		movie->LastError = gf_odf_desc_copy((GF_Descriptor *) movie->moov->iods->descriptor, &desc);
		if (movie->LastError) return NULL;
		return desc;
	}
	od = NULL;
	iod = NULL;

	switch (movie->moov->iods->descriptor->tag) {
	case GF_ODF_ISOM_OD_TAG:
		od = (GF_ObjectDescriptor*)gf_malloc(sizeof(GF_ObjectDescriptor));
		if (!od) return NULL;

		memset(od, 0, sizeof(GF_ObjectDescriptor));
		od->ESDescriptors = gf_list_new();
		useIOD = 0;
		break;
	case GF_ODF_ISOM_IOD_TAG:
		iod = (GF_InitialObjectDescriptor*)gf_malloc(sizeof(GF_InitialObjectDescriptor));
		if (!iod) return NULL;

		memset(iod, 0, sizeof(GF_InitialObjectDescriptor));
		iod->ESDescriptors = gf_list_new();
		useIOD = 1;
		break;
	default:
		return NULL;
	}

	//duplicate our descriptor
	movie->LastError = gf_odf_desc_copy((GF_Descriptor *) movie->moov->iods->descriptor, &desc);
	if (movie->LastError) return NULL;

	if (!useIOD) {
		isom_od = (GF_IsomObjectDescriptor *)desc;
		od->objectDescriptorID = isom_od->objectDescriptorID;
		od->extensionDescriptors = isom_od->extensionDescriptors;
		isom_od->extensionDescriptors = NULL;
		od->IPMP_Descriptors = isom_od->IPMP_Descriptors;
		isom_od->IPMP_Descriptors = NULL;
		od->OCIDescriptors = isom_od->OCIDescriptors;
		isom_od->OCIDescriptors = NULL;
		od->URLString = isom_od->URLString;
		isom_od->URLString = NULL;
		od->tag = GF_ODF_OD_TAG;
		//then recreate the desc in Inc
		i=0;
		while ((inc = (GF_ES_ID_Inc*)gf_list_enum(isom_od->ES_ID_IncDescriptors, &i))) {
			movie->LastError = GetESDForTime(movie->moov, inc->trackID, 0, &esd);
			if (!movie->LastError) movie->LastError = gf_list_add(od->ESDescriptors, esd);
			if (movie->LastError) {
				gf_odf_desc_del(desc);
				gf_odf_desc_del((GF_Descriptor *) od);
				return NULL;
			}
		}
		gf_odf_desc_del(desc);
		return (GF_Descriptor *)od;
	} else {
		isom_iod = (GF_IsomInitialObjectDescriptor *)desc;
		iod->objectDescriptorID = isom_iod->objectDescriptorID;
		iod->extensionDescriptors = isom_iod->extensionDescriptors;
		isom_iod->extensionDescriptors = NULL;
		iod->IPMP_Descriptors = isom_iod->IPMP_Descriptors;
		isom_iod->IPMP_Descriptors = NULL;
		iod->OCIDescriptors = isom_iod->OCIDescriptors;
		isom_iod->OCIDescriptors = NULL;
		iod->URLString = isom_iod->URLString;
		isom_iod->URLString = NULL;
		iod->tag = GF_ODF_IOD_TAG;

		iod->audio_profileAndLevel = isom_iod->audio_profileAndLevel;
		iod->graphics_profileAndLevel = isom_iod->graphics_profileAndLevel;
		iod->inlineProfileFlag = isom_iod->inlineProfileFlag;
		iod->OD_profileAndLevel = isom_iod->OD_profileAndLevel;
		iod->scene_profileAndLevel = isom_iod->scene_profileAndLevel;
		iod->visual_profileAndLevel = isom_iod->visual_profileAndLevel;
		iod->IPMPToolList = isom_iod->IPMPToolList;
		isom_iod->IPMPToolList = NULL;

		//then recreate the desc in Inc
		i=0;
		while ((inc = (GF_ES_ID_Inc*)gf_list_enum(isom_iod->ES_ID_IncDescriptors, &i))) {
			movie->LastError = GetESDForTime(movie->moov, inc->trackID, 0, &esd);
			if (!movie->LastError) movie->LastError = gf_list_add(iod->ESDescriptors, esd);
			if (movie->LastError) {
				gf_odf_desc_del(desc);
				gf_odf_desc_del((GF_Descriptor *) iod);
				return NULL;
			}
		}
		gf_odf_desc_del(desc);
		return (GF_Descriptor *)iod;
	}
}