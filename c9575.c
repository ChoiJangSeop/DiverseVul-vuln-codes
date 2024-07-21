void __init xen_init_IRQ(void)
{
	int ret = -EINVAL;
	evtchn_port_t evtchn;

	if (fifo_events)
		ret = xen_evtchn_fifo_init();
	if (ret < 0)
		xen_evtchn_2l_init();

	cpuhp_setup_state_nocalls(CPUHP_XEN_EVTCHN_PREPARE,
				  "xen/evtchn:prepare",
				  xen_evtchn_cpu_prepare, xen_evtchn_cpu_dead);

	evtchn_to_irq = kcalloc(EVTCHN_ROW(xen_evtchn_max_channels()),
				sizeof(*evtchn_to_irq), GFP_KERNEL);
	BUG_ON(!evtchn_to_irq);

	/* No event channels are 'live' right now. */
	for (evtchn = 0; evtchn < xen_evtchn_nr_channels(); evtchn++)
		mask_evtchn(evtchn);

	pirq_needs_eoi = pirq_needs_eoi_flag;

#ifdef CONFIG_X86
	if (xen_pv_domain()) {
		if (xen_initial_domain())
			pci_xen_initial_domain();
	}
	if (xen_feature(XENFEAT_hvm_callback_vector)) {
		xen_setup_callback_vector();
		xen_alloc_callback_vector();
	}

	if (xen_hvm_domain()) {
		native_init_IRQ();
		/* pci_xen_hvm_init must be called after native_init_IRQ so that
		 * __acpi_register_gsi can point at the right function */
		pci_xen_hvm_init();
	} else {
		int rc;
		struct physdev_pirq_eoi_gmfn eoi_gmfn;

		pirq_eoi_map = (void *)__get_free_page(GFP_KERNEL|__GFP_ZERO);
		eoi_gmfn.gmfn = virt_to_gfn(pirq_eoi_map);
		rc = HYPERVISOR_physdev_op(PHYSDEVOP_pirq_eoi_gmfn_v2, &eoi_gmfn);
		if (rc != 0) {
			free_page((unsigned long) pirq_eoi_map);
			pirq_eoi_map = NULL;
		} else
			pirq_needs_eoi = pirq_check_eoi_map;
	}
#endif
}