struct l2tp_packet_t *l2tp_packet_alloc(int ver, int msg_type,
					const struct sockaddr_in *addr, int H,
					const char *secret, size_t secret_len)
{
	struct l2tp_packet_t *pack = mempool_alloc(pack_pool);
	if (!pack)
		return NULL;

	memset(pack, 0, sizeof(*pack));
	INIT_LIST_HEAD(&pack->attrs);
	pack->hdr.ver = ver;
	pack->hdr.T = 1;
	pack->hdr.L = 1;
	pack->hdr.S = 1;
	memcpy(&pack->addr, addr, sizeof(*addr));
	pack->hide_avps = H;
	pack->secret = secret;
	pack->secret_len = secret_len;

	if (msg_type) {
		if (l2tp_packet_add_int16(pack, Message_Type, msg_type, 1)) {
			mempool_free(pack);
			return NULL;
		}
	}

	return pack;
}