iter_dns_store(struct module_env* env, struct query_info* msgqinf,
	struct reply_info* msgrep, int is_referral, time_t leeway, int pside,
	struct regional* region, uint16_t flags)
{
	if(!dns_cache_store(env, msgqinf, msgrep, is_referral, leeway,
		pside, region, flags))
		log_err("out of memory: cannot store data in cache");
}