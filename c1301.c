new_msg_originate_request (u_int32_t seqnum,
			   struct in_addr ifaddr,
			   struct in_addr area_id, struct lsa_header *data)
{
  struct msg_originate_request *omsg;
  int omsglen;
  char buf[OSPF_API_MAX_MSG_SIZE];

  omsglen = sizeof (struct msg_originate_request) - sizeof (struct lsa_header)
    + ntohs (data->length);

  omsg = (struct msg_originate_request *) buf;
  omsg->ifaddr = ifaddr;
  omsg->area_id = area_id;
  memcpy (&omsg->data, data, ntohs (data->length));

  return msg_new (MSG_ORIGINATE_REQUEST, omsg, seqnum, omsglen);
}