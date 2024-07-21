lldpd_alloc_mgmt(int family, void *addrptr, size_t addrsize, u_int32_t iface)
{
	struct lldpd_mgmt *mgmt;

	log_debug("alloc", "allocate a new management address (family: %d)", family);

	if (family <= LLDPD_AF_UNSPEC || family >= LLDPD_AF_LAST) {
		errno = EAFNOSUPPORT;
		return NULL;
	}
	if (addrsize > LLDPD_MGMT_MAXADDRSIZE) {
		errno = EOVERFLOW;
		return NULL;
	}
	mgmt = calloc(1, sizeof(struct lldpd_mgmt));
	if (mgmt == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	mgmt->m_family = family;
	assert(addrsize <= LLDPD_MGMT_MAXADDRSIZE);
	memcpy(&mgmt->m_addr, addrptr, addrsize);
	mgmt->m_addrsize = addrsize;
	mgmt->m_iface = iface;
	return mgmt;
}