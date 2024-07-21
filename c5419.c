static int parse_rr(unsigned char *buf, unsigned char *start,
			unsigned char *max,
			unsigned char *response, unsigned int *response_size,
			uint16_t *type, uint16_t *class, int *ttl, int *rdlen,
			unsigned char **end,
			char *name)
{
	struct domain_rr *rr;
	int err, offset;
	int name_len = 0, output_len = 0, max_rsp = *response_size;

	err = get_name(0, buf, start, max, response, max_rsp,
		&output_len, end, name, &name_len);
	if (err < 0)
		return err;

	offset = output_len;

	if ((unsigned int) offset > *response_size)
		return -ENOBUFS;

	rr = (void *) (*end);

	if (!rr)
		return -EINVAL;

	*type = ntohs(rr->type);
	*class = ntohs(rr->class);
	*ttl = ntohl(rr->ttl);
	*rdlen = ntohs(rr->rdlen);

	if (*ttl < 0)
		return -EINVAL;

	memcpy(response + offset, *end, sizeof(struct domain_rr));

	offset += sizeof(struct domain_rr);
	*end += sizeof(struct domain_rr);

	if ((unsigned int) (offset + *rdlen) > *response_size)
		return -ENOBUFS;

	memcpy(response + offset, *end, *rdlen);

	*end += *rdlen;

	*response_size = offset + *rdlen;

	return 0;
}