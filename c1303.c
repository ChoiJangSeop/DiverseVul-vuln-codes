new_msg_sync_lsdb (u_int32_t seqnum, struct lsa_filter_type *filter)
{
  u_char buf[OSPF_API_MAX_MSG_SIZE];
  struct msg_sync_lsdb *smsg;
  int len;

  smsg = (struct msg_sync_lsdb *) buf;
  len = sizeof (struct msg_sync_lsdb) +
    filter->num_areas * sizeof (struct in_addr);
  smsg->filter.typemask = htons (filter->typemask);
  smsg->filter.origin = filter->origin;
  smsg->filter.num_areas = filter->num_areas;
  return msg_new (MSG_SYNC_LSDB, smsg, seqnum, len);
}