static int install_relocation_handler(int num_cpus, size_t save_state_size)
{
	struct smm_loader_params smm_params = {
		.per_cpu_stack_size = CONFIG_SMM_STUB_STACK_SIZE,
		.num_concurrent_stacks = num_cpus,
		.per_cpu_save_state_size = save_state_size,
		.num_concurrent_save_states = 1,
		.handler = smm_do_relocation,
	};

	/* Allow callback to override parameters. */
	if (mp_state.ops.adjust_smm_params != NULL)
		mp_state.ops.adjust_smm_params(&smm_params, 0);

	if (smm_setup_relocation_handler(&smm_params))
		return -1;

	adjust_smm_apic_id_map(&smm_params);

	return 0;
}