int l2tp_recv(int fd, struct l2tp_packet_t **p, struct in_pktinfo *pkt_info,
	      const char *secret, size_t secret_len)
{
	int n, length;
	uint8_t *buf;
	struct l2tp_hdr_t *hdr;
	struct l2tp_avp_t *avp;
	struct l2tp_dict_attr_t *da;
	struct l2tp_attr_t *attr, *RV = NULL;
	uint8_t *ptr;
	struct l2tp_packet_t *pack;
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	struct msghdr msg;
	char msg_control[128];
	struct cmsghdr *cmsg;
	uint16_t orig_avp_len;
	void *orig_avp_val;

  *p = NULL;

	if (pkt_info) {
		memset(&msg, 0, sizeof(msg));
		msg.msg_control = msg_control;
		msg.msg_controllen = 128;

		n = recvmsg(fd, &msg, MSG_PEEK);

		if (n < 0) {
			if (errno == EAGAIN)
				return -1;
			log_error("l2tp: recvmsg: %s\n", strerror(errno));
			return 0;
		}

		for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
			if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO) {
				memcpy(pkt_info, CMSG_DATA(cmsg), sizeof(*pkt_info));
				break;
			}
		}
	}

	buf = mempool_alloc(buf_pool);
	if (!buf) {
		log_emerg("l2tp: out of memory\n");
		return 0;
	}
	hdr = (struct l2tp_hdr_t *)buf;
	ptr = (uint8_t *)(hdr + 1);

	n = recvfrom(fd, buf, L2TP_MAX_PACKET_SIZE, 0, &addr, &len);

	if (n < 0) {
		mempool_free(buf);
		if (errno == EAGAIN) {
			return -1;
		} else if (errno == ECONNREFUSED) {
			return -2;
		}
		log_error("l2tp: recv: %s\n", strerror(errno));
		return 0;
	}

	if (n < 6) {
		if (conf_verbose)
			log_warn("l2tp: short packet received (%i/%zu)\n", n, sizeof(*hdr));
		goto out_err_hdr;
	}

	if (hdr->T == 0)
		goto out_err_hdr;

	if (n < ntohs(hdr->length)) {
		if (conf_verbose)
			log_warn("l2tp: short packet received (%i/%i)\n", n, ntohs(hdr->length));
		goto out_err_hdr;
	}

	if (hdr->ver == 2) {
		if (hdr->L == 0) {
			if (conf_verbose)
				log_warn("l2tp: incorrect message received (L=0)\n");
			if (!conf_avp_permissive)
			    goto out_err_hdr;
		}

		if (hdr->S == 0) {
			if (conf_verbose)
				log_warn("l2tp: incorrect message received (S=0)\n");
			if (!conf_avp_permissive)
			    goto out_err_hdr;
		}

		if (hdr->O == 1) {
			if (conf_verbose)
				log_warn("l2tp: incorrect message received (O=1)\n");
			if (!conf_avp_permissive)
			    goto out_err_hdr;
		}
	} else if (hdr->ver != 3) {
		if (conf_verbose)
			log_warn("l2tp: protocol version %i is not supported\n", hdr->ver);
		goto out_err_hdr;
	}

	pack = mempool_alloc(pack_pool);
	if (!pack) {
		log_emerg("l2tp: out of memory\n");
		goto out_err_hdr;
	}

	memset(pack, 0, sizeof(*pack));
	INIT_LIST_HEAD(&pack->attrs);

	memcpy(&pack->addr, &addr, sizeof(addr));
	memcpy(&pack->hdr, hdr, sizeof(*hdr));
	length = ntohs(hdr->length) - sizeof(*hdr);

	while (length) {
		*(uint16_t *)ptr = ntohs(*(uint16_t *)ptr);
		avp = (struct l2tp_avp_t *)ptr;

		if (avp->length > length) {
			if (conf_verbose)
				log_warn("l2tp: incorrect avp received (exceeds message length)\n");
			goto out_err;
		}

		if (avp->vendor)
			goto skip;

		da = l2tp_dict_find_attr_by_id(ntohs(avp->type));
		if (!da) {
			if (conf_verbose)
				log_warn("l2tp: unknown avp received (type=%i, M=%u)\n", ntohs(avp->type), avp->M);
			if (avp->M && !conf_avp_permissive)
				goto out_err;
		} else {
			if (da->M != -1 && da->M != avp->M) {
				if (conf_verbose)
					log_warn("l2tp: incorrect avp received (type=%i, M=%i, must be %i)\n", ntohs(avp->type), avp->M, da->M);
				if (!conf_avp_permissive)
				    goto out_err;
			}

			if (da->H != -1 && da->H != avp->H) {
				if (conf_verbose)
					log_warn("l2tp: incorrect avp received (type=%i, H=%i, must be %i)\n", ntohs(avp->type), avp->H, da->H);
				if (!conf_avp_permissive)
				    goto out_err;
			}

			if (avp->H) {
				if (!RV) {
					if (conf_verbose)
						log_warn("l2tp: incorrect avp received (type=%i, H=1, but Random-Vector is not received)\n", ntohs(avp->type));
					goto out_err;
				}
				if (secret == NULL || secret_len == 0) {
					log_error("l2tp: impossible to decode"
						  " hidden avp (type %hu): no"
						  " secret set)\n",
						  ntohs(avp->type));
					goto out_err;
				}
				if (decode_avp(avp, RV, secret, secret_len) < 0)
					goto out_err;
			}

			attr = mempool_alloc(attr_pool);
			memset(attr, 0, sizeof(*attr));
			list_add_tail(&attr->entry, &pack->attrs);

			if (avp->H) {
				orig_avp_len = ntohs(*(uint16_t *)avp->val) + sizeof(*avp);
				orig_avp_val = avp->val + sizeof(uint16_t);
			} else {
				orig_avp_len = avp->length;
				orig_avp_val = avp->val;
			}

			attr->attr = da;
			attr->M = avp->M;
			attr->H = 0;
			attr->length = orig_avp_len - sizeof(*avp);

			if (attr->attr->id == Random_Vector)
				RV = attr;

			switch (da->type) {
				case ATTR_TYPE_INT16:
					if (orig_avp_len != sizeof(*avp) + 2)
						goto out_err_len;
					attr->val.uint16 = ntohs(*(uint16_t *)orig_avp_val);
					break;
				case ATTR_TYPE_INT32:
					if (orig_avp_len != sizeof(*avp) + 4)
						goto out_err_len;
					attr->val.uint32 = ntohl(*(uint32_t *)orig_avp_val);
					break;
				case ATTR_TYPE_INT64:
					if (orig_avp_len != sizeof(*avp) + 8)
						goto out_err_len;
					attr->val.uint64 = be64toh(*(uint64_t *)orig_avp_val);
					break;
				case ATTR_TYPE_OCTETS:
					attr->val.octets = _malloc(attr->length);
					if (!attr->val.octets)
						goto out_err_mem;
					memcpy(attr->val.octets, orig_avp_val, attr->length);
					break;
				case ATTR_TYPE_STRING:
					attr->val.string = _malloc(attr->length + 1);
					if (!attr->val.string)
						goto out_err_mem;
					memcpy(attr->val.string, orig_avp_val, attr->length);
					attr->val.string[attr->length] = 0;
					break;
			}
		}
skip:
		ptr += avp->length;
		length -= avp->length;
	}

	*p = pack;

	mempool_free(buf);

	return 0;

out_err:
	l2tp_packet_free(pack);
out_err_hdr:
	mempool_free(buf);
	return 0;
out_err_len:
	if (conf_verbose)
		log_warn("l2tp: incorrect avp received (type=%i, incorrect length %i)\n", ntohs(avp->type), orig_avp_len);
	goto out_err;
out_err_mem:
	log_emerg("l2tp: out of memory\n");
	goto out_err;
}