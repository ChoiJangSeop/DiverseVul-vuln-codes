static int pf_detect(void)
{
	struct pf_unit *pf = units;
	int k, unit;

	printk("%s: %s version %s, major %d, cluster %d, nice %d\n",
	       name, name, PF_VERSION, major, cluster, nice);

	par_drv = pi_register_driver(name);
	if (!par_drv) {
		pr_err("failed to register %s driver\n", name);
		return -1;
	}
	k = 0;
	if (pf_drive_count == 0) {
		if (pi_init(pf->pi, 1, -1, -1, -1, -1, -1, pf_scratch, PI_PF,
			    verbose, pf->name)) {
			if (!pf_probe(pf) && pf->disk) {
				pf->present = 1;
				k++;
			} else
				pi_release(pf->pi);
		}

	} else
		for (unit = 0; unit < PF_UNITS; unit++, pf++) {
			int *conf = *drives[unit];
			if (!conf[D_PRT])
				continue;
			if (pi_init(pf->pi, 0, conf[D_PRT], conf[D_MOD],
				    conf[D_UNI], conf[D_PRO], conf[D_DLY],
				    pf_scratch, PI_PF, verbose, pf->name)) {
				if (pf->disk && !pf_probe(pf)) {
					pf->present = 1;
					k++;
				} else
					pi_release(pf->pi);
			}
		}
	if (k)
		return 0;

	printk("%s: No ATAPI disk detected\n", name);
	for (pf = units, unit = 0; unit < PF_UNITS; pf++, unit++) {
		blk_cleanup_queue(pf->disk->queue);
		pf->disk->queue = NULL;
		blk_mq_free_tag_set(&pf->tag_set);
		put_disk(pf->disk);
	}
	pi_unregister_driver(par_drv);
	return -1;
}