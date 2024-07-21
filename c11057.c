int __init ip_rt_init(void)
{
	int cpu;

	ip_idents = kmalloc_array(IP_IDENTS_SZ, sizeof(*ip_idents),
				  GFP_KERNEL);
	if (!ip_idents)
		panic("IP: failed to allocate ip_idents\n");

	prandom_bytes(ip_idents, IP_IDENTS_SZ * sizeof(*ip_idents));

	ip_tstamps = kcalloc(IP_IDENTS_SZ, sizeof(*ip_tstamps), GFP_KERNEL);
	if (!ip_tstamps)
		panic("IP: failed to allocate ip_tstamps\n");

	for_each_possible_cpu(cpu) {
		struct uncached_list *ul = &per_cpu(rt_uncached_list, cpu);

		INIT_LIST_HEAD(&ul->head);
		spin_lock_init(&ul->lock);
	}
#ifdef CONFIG_IP_ROUTE_CLASSID
	ip_rt_acct = __alloc_percpu(256 * sizeof(struct ip_rt_acct), __alignof__(struct ip_rt_acct));
	if (!ip_rt_acct)
		panic("IP: failed to allocate ip_rt_acct\n");
#endif

	ipv4_dst_ops.kmem_cachep =
		kmem_cache_create("ip_dst_cache", sizeof(struct rtable), 0,
				  SLAB_HWCACHE_ALIGN|SLAB_PANIC, NULL);

	ipv4_dst_blackhole_ops.kmem_cachep = ipv4_dst_ops.kmem_cachep;

	if (dst_entries_init(&ipv4_dst_ops) < 0)
		panic("IP: failed to allocate ipv4_dst_ops counter\n");

	if (dst_entries_init(&ipv4_dst_blackhole_ops) < 0)
		panic("IP: failed to allocate ipv4_dst_blackhole_ops counter\n");

	ipv4_dst_ops.gc_thresh = ~0;
	ip_rt_max_size = INT_MAX;

	devinet_init();
	ip_fib_init();

	if (ip_rt_proc_init())
		pr_err("Unable to create route proc files\n");
#ifdef CONFIG_XFRM
	xfrm_init();
	xfrm4_init();
#endif
	rtnl_register(PF_INET, RTM_GETROUTE, inet_rtm_getroute, NULL,
		      RTNL_FLAG_DOIT_UNLOCKED);

#ifdef CONFIG_SYSCTL
	register_pernet_subsys(&sysctl_route_ops);
#endif
	register_pernet_subsys(&rt_genid_ops);
	register_pernet_subsys(&ipv4_inetpeer_ops);
	return 0;
}