static ssize_t node_read_meminfo(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int n;
	int nid = dev->id;
	struct pglist_data *pgdat = NODE_DATA(nid);
	struct sysinfo i;
	unsigned long sreclaimable, sunreclaimable;

	si_meminfo_node(&i, nid);
	sreclaimable = node_page_state_pages(pgdat, NR_SLAB_RECLAIMABLE_B);
	sunreclaimable = node_page_state_pages(pgdat, NR_SLAB_UNRECLAIMABLE_B);
	n = sprintf(buf,
		       "Node %d MemTotal:       %8lu kB\n"
		       "Node %d MemFree:        %8lu kB\n"
		       "Node %d MemUsed:        %8lu kB\n"
		       "Node %d Active:         %8lu kB\n"
		       "Node %d Inactive:       %8lu kB\n"
		       "Node %d Active(anon):   %8lu kB\n"
		       "Node %d Inactive(anon): %8lu kB\n"
		       "Node %d Active(file):   %8lu kB\n"
		       "Node %d Inactive(file): %8lu kB\n"
		       "Node %d Unevictable:    %8lu kB\n"
		       "Node %d Mlocked:        %8lu kB\n",
		       nid, K(i.totalram),
		       nid, K(i.freeram),
		       nid, K(i.totalram - i.freeram),
		       nid, K(node_page_state(pgdat, NR_ACTIVE_ANON) +
				node_page_state(pgdat, NR_ACTIVE_FILE)),
		       nid, K(node_page_state(pgdat, NR_INACTIVE_ANON) +
				node_page_state(pgdat, NR_INACTIVE_FILE)),
		       nid, K(node_page_state(pgdat, NR_ACTIVE_ANON)),
		       nid, K(node_page_state(pgdat, NR_INACTIVE_ANON)),
		       nid, K(node_page_state(pgdat, NR_ACTIVE_FILE)),
		       nid, K(node_page_state(pgdat, NR_INACTIVE_FILE)),
		       nid, K(node_page_state(pgdat, NR_UNEVICTABLE)),
		       nid, K(sum_zone_node_page_state(nid, NR_MLOCK)));

#ifdef CONFIG_HIGHMEM
	n += sprintf(buf + n,
		       "Node %d HighTotal:      %8lu kB\n"
		       "Node %d HighFree:       %8lu kB\n"
		       "Node %d LowTotal:       %8lu kB\n"
		       "Node %d LowFree:        %8lu kB\n",
		       nid, K(i.totalhigh),
		       nid, K(i.freehigh),
		       nid, K(i.totalram - i.totalhigh),
		       nid, K(i.freeram - i.freehigh));
#endif
	n += sprintf(buf + n,
		       "Node %d Dirty:          %8lu kB\n"
		       "Node %d Writeback:      %8lu kB\n"
		       "Node %d FilePages:      %8lu kB\n"
		       "Node %d Mapped:         %8lu kB\n"
		       "Node %d AnonPages:      %8lu kB\n"
		       "Node %d Shmem:          %8lu kB\n"
		       "Node %d KernelStack:    %8lu kB\n"
#ifdef CONFIG_SHADOW_CALL_STACK
		       "Node %d ShadowCallStack:%8lu kB\n"
#endif
		       "Node %d PageTables:     %8lu kB\n"
		       "Node %d NFS_Unstable:   %8lu kB\n"
		       "Node %d Bounce:         %8lu kB\n"
		       "Node %d WritebackTmp:   %8lu kB\n"
		       "Node %d KReclaimable:   %8lu kB\n"
		       "Node %d Slab:           %8lu kB\n"
		       "Node %d SReclaimable:   %8lu kB\n"
		       "Node %d SUnreclaim:     %8lu kB\n"
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
		       "Node %d AnonHugePages:  %8lu kB\n"
		       "Node %d ShmemHugePages: %8lu kB\n"
		       "Node %d ShmemPmdMapped: %8lu kB\n"
		       "Node %d FileHugePages: %8lu kB\n"
		       "Node %d FilePmdMapped: %8lu kB\n"
#endif
			,
		       nid, K(node_page_state(pgdat, NR_FILE_DIRTY)),
		       nid, K(node_page_state(pgdat, NR_WRITEBACK)),
		       nid, K(node_page_state(pgdat, NR_FILE_PAGES)),
		       nid, K(node_page_state(pgdat, NR_FILE_MAPPED)),
		       nid, K(node_page_state(pgdat, NR_ANON_MAPPED)),
		       nid, K(i.sharedram),
		       nid, node_page_state(pgdat, NR_KERNEL_STACK_KB),
#ifdef CONFIG_SHADOW_CALL_STACK
		       nid, node_page_state(pgdat, NR_KERNEL_SCS_KB),
#endif
		       nid, K(sum_zone_node_page_state(nid, NR_PAGETABLE)),
		       nid, 0UL,
		       nid, K(sum_zone_node_page_state(nid, NR_BOUNCE)),
		       nid, K(node_page_state(pgdat, NR_WRITEBACK_TEMP)),
		       nid, K(sreclaimable +
			      node_page_state(pgdat, NR_KERNEL_MISC_RECLAIMABLE)),
		       nid, K(sreclaimable + sunreclaimable),
		       nid, K(sreclaimable),
		       nid, K(sunreclaimable)
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
		       ,
		       nid, K(node_page_state(pgdat, NR_ANON_THPS) *
				       HPAGE_PMD_NR),
		       nid, K(node_page_state(pgdat, NR_SHMEM_THPS) *
				       HPAGE_PMD_NR),
		       nid, K(node_page_state(pgdat, NR_SHMEM_PMDMAPPED) *
				       HPAGE_PMD_NR),
		       nid, K(node_page_state(pgdat, NR_FILE_THPS) *
				       HPAGE_PMD_NR),
		       nid, K(node_page_state(pgdat, NR_FILE_PMDMAPPED) *
				       HPAGE_PMD_NR)
#endif
		       );
	n += hugetlb_report_node_meminfo(nid, buf + n);
	return n;
}