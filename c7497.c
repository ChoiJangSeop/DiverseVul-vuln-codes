int megasas_alloc_cmds(struct megasas_instance *instance)
{
	int i;
	int j;
	u16 max_cmd;
	struct megasas_cmd *cmd;

	max_cmd = instance->max_mfi_cmds;

	/*
	 * instance->cmd_list is an array of struct megasas_cmd pointers.
	 * Allocate the dynamic array first and then allocate individual
	 * commands.
	 */
	instance->cmd_list = kcalloc(max_cmd, sizeof(struct megasas_cmd*), GFP_KERNEL);

	if (!instance->cmd_list) {
		dev_printk(KERN_DEBUG, &instance->pdev->dev, "out of memory\n");
		return -ENOMEM;
	}

	memset(instance->cmd_list, 0, sizeof(struct megasas_cmd *) *max_cmd);

	for (i = 0; i < max_cmd; i++) {
		instance->cmd_list[i] = kmalloc(sizeof(struct megasas_cmd),
						GFP_KERNEL);

		if (!instance->cmd_list[i]) {

			for (j = 0; j < i; j++)
				kfree(instance->cmd_list[j]);

			kfree(instance->cmd_list);
			instance->cmd_list = NULL;

			return -ENOMEM;
		}
	}

	for (i = 0; i < max_cmd; i++) {
		cmd = instance->cmd_list[i];
		memset(cmd, 0, sizeof(struct megasas_cmd));
		cmd->index = i;
		cmd->scmd = NULL;
		cmd->instance = instance;

		list_add_tail(&cmd->list, &instance->cmd_pool);
	}

	/*
	 * Create a frame pool and assign one frame to each cmd
	 */
	if (megasas_create_frame_pool(instance)) {
		dev_printk(KERN_DEBUG, &instance->pdev->dev, "Error creating frame DMA pool\n");
		megasas_free_cmds(instance);
	}

	return 0;
}