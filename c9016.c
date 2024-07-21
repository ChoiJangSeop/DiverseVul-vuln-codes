smart_compare(sldns_buffer* pkt, uint8_t* dnow, 
	uint8_t* dprfirst, uint8_t* dprlast)
{
	if(LABEL_IS_PTR(*dnow)) {
		/* ptr points to a previous dname */
		uint8_t* p = sldns_buffer_at(pkt, PTR_OFFSET(dnow[0], dnow[1]));
		if( p == dprfirst || p == dprlast )
			return 0;
		/* prev dname is also a ptr, both ptrs are the same. */
		if(LABEL_IS_PTR(*dprlast) &&
			dprlast[0] == dnow[0] && dprlast[1] == dnow[1])
			return 0;
	}
	return dname_pkt_compare(pkt, dnow, dprlast);
}