static int try_smi_init(struct smi_info *new_smi)
{
	int rv = 0;
	int i;
	char *init_name = NULL;

	pr_info("Trying %s-specified %s state machine at %s address 0x%lx, slave address 0x%x, irq %d\n",
		ipmi_addr_src_to_str(new_smi->io.addr_source),
		si_to_str[new_smi->io.si_type],
		addr_space_to_str[new_smi->io.addr_type],
		new_smi->io.addr_data,
		new_smi->io.slave_addr, new_smi->io.irq);

	switch (new_smi->io.si_type) {
	case SI_KCS:
		new_smi->handlers = &kcs_smi_handlers;
		break;

	case SI_SMIC:
		new_smi->handlers = &smic_smi_handlers;
		break;

	case SI_BT:
		new_smi->handlers = &bt_smi_handlers;
		break;

	default:
		/* No support for anything else yet. */
		rv = -EIO;
		goto out_err;
	}

	new_smi->si_num = smi_num;

	/* Do this early so it's available for logs. */
	if (!new_smi->io.dev) {
		init_name = kasprintf(GFP_KERNEL, "ipmi_si.%d",
				      new_smi->si_num);

		/*
		 * If we don't already have a device from something
		 * else (like PCI), then register a new one.
		 */
		new_smi->pdev = platform_device_alloc("ipmi_si",
						      new_smi->si_num);
		if (!new_smi->pdev) {
			pr_err("Unable to allocate platform device\n");
			rv = -ENOMEM;
			goto out_err;
		}
		new_smi->io.dev = &new_smi->pdev->dev;
		new_smi->io.dev->driver = &ipmi_platform_driver.driver;
		/* Nulled by device_add() */
		new_smi->io.dev->init_name = init_name;
	}

	/* Allocate the state machine's data and initialize it. */
	new_smi->si_sm = kmalloc(new_smi->handlers->size(), GFP_KERNEL);
	if (!new_smi->si_sm) {
		rv = -ENOMEM;
		goto out_err;
	}
	new_smi->io.io_size = new_smi->handlers->init_data(new_smi->si_sm,
							   &new_smi->io);

	/* Now that we know the I/O size, we can set up the I/O. */
	rv = new_smi->io.io_setup(&new_smi->io);
	if (rv) {
		dev_err(new_smi->io.dev, "Could not set up I/O space\n");
		goto out_err;
	}

	/* Do low-level detection first. */
	if (new_smi->handlers->detect(new_smi->si_sm)) {
		if (new_smi->io.addr_source)
			dev_err(new_smi->io.dev,
				"Interface detection failed\n");
		rv = -ENODEV;
		goto out_err;
	}

	/*
	 * Attempt a get device id command.  If it fails, we probably
	 * don't have a BMC here.
	 */
	rv = try_get_dev_id(new_smi);
	if (rv) {
		if (new_smi->io.addr_source)
			dev_err(new_smi->io.dev,
			       "There appears to be no BMC at this location\n");
		goto out_err;
	}

	setup_oem_data_handler(new_smi);
	setup_xaction_handlers(new_smi);
	check_for_broken_irqs(new_smi);

	new_smi->waiting_msg = NULL;
	new_smi->curr_msg = NULL;
	atomic_set(&new_smi->req_events, 0);
	new_smi->run_to_completion = false;
	for (i = 0; i < SI_NUM_STATS; i++)
		atomic_set(&new_smi->stats[i], 0);

	new_smi->interrupt_disabled = true;
	atomic_set(&new_smi->need_watch, 0);

	rv = try_enable_event_buffer(new_smi);
	if (rv == 0)
		new_smi->has_event_buffer = true;

	/*
	 * Start clearing the flags before we enable interrupts or the
	 * timer to avoid racing with the timer.
	 */
	start_clear_flags(new_smi);

	/*
	 * IRQ is defined to be set when non-zero.  req_events will
	 * cause a global flags check that will enable interrupts.
	 */
	if (new_smi->io.irq) {
		new_smi->interrupt_disabled = false;
		atomic_set(&new_smi->req_events, 1);
	}

	if (new_smi->pdev && !new_smi->pdev_registered) {
		rv = platform_device_add(new_smi->pdev);
		if (rv) {
			dev_err(new_smi->io.dev,
				"Unable to register system interface device: %d\n",
				rv);
			goto out_err;
		}
		new_smi->pdev_registered = true;
	}

	dev_set_drvdata(new_smi->io.dev, new_smi);
	rv = device_add_group(new_smi->io.dev, &ipmi_si_dev_attr_group);
	if (rv) {
		dev_err(new_smi->io.dev,
			"Unable to add device attributes: error %d\n",
			rv);
		goto out_err;
	}
	new_smi->dev_group_added = true;

	rv = ipmi_register_smi(&handlers,
			       new_smi,
			       new_smi->io.dev,
			       new_smi->io.slave_addr);
	if (rv) {
		dev_err(new_smi->io.dev,
			"Unable to register device: error %d\n",
			rv);
		goto out_err;
	}

	/* Don't increment till we know we have succeeded. */
	smi_num++;

	dev_info(new_smi->io.dev, "IPMI %s interface initialized\n",
		 si_to_str[new_smi->io.si_type]);

	WARN_ON(new_smi->io.dev->init_name != NULL);

 out_err:
	kfree(init_name);
	return rv;
}