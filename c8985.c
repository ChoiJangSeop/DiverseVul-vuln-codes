void delegpt_add_neg_msg(struct delegpt* dp, struct msgreply_entry* msg)
{
	struct reply_info* rep = (struct reply_info*)msg->entry.data;
	if(!rep) return;

	/* if error or no answers */
	if(FLAGS_GET_RCODE(rep->flags) != 0 || rep->an_numrrsets == 0) {
		struct delegpt_ns* ns = delegpt_find_ns(dp, msg->key.qname, 
			msg->key.qname_len);
		if(ns) {
			if(msg->key.qtype == LDNS_RR_TYPE_A)
				ns->got4 = 1;
			else if(msg->key.qtype == LDNS_RR_TYPE_AAAA)
				ns->got6 = 1;
			if(ns->got4 && ns->got6)
				ns->resolved = 1;
		}
	}
}