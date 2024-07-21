static void sas_eh_handle_sas_errors(struct Scsi_Host *shost, struct list_head *work_q)
{
	struct scsi_cmnd *cmd, *n;
	enum task_disposition res = TASK_IS_DONE;
	int tmf_resp, need_reset;
	struct sas_internal *i = to_sas_internal(shost->transportt);
	unsigned long flags;
	struct sas_ha_struct *ha = SHOST_TO_SAS_HA(shost);
	LIST_HEAD(done);

	/* clean out any commands that won the completion vs eh race */
	list_for_each_entry_safe(cmd, n, work_q, eh_entry) {
		struct domain_device *dev = cmd_to_domain_dev(cmd);
		struct sas_task *task;

		spin_lock_irqsave(&dev->done_lock, flags);
		/* by this point the lldd has either observed
		 * SAS_HA_FROZEN and is leaving the task alone, or has
		 * won the race with eh and decided to complete it
		 */
		task = TO_SAS_TASK(cmd);
		spin_unlock_irqrestore(&dev->done_lock, flags);

		if (!task)
			list_move_tail(&cmd->eh_entry, &done);
	}

 Again:
	list_for_each_entry_safe(cmd, n, work_q, eh_entry) {
		struct sas_task *task = TO_SAS_TASK(cmd);

		list_del_init(&cmd->eh_entry);

		spin_lock_irqsave(&task->task_state_lock, flags);
		need_reset = task->task_state_flags & SAS_TASK_NEED_DEV_RESET;
		spin_unlock_irqrestore(&task->task_state_lock, flags);

		if (need_reset) {
			SAS_DPRINTK("%s: task 0x%p requests reset\n",
				    __func__, task);
			goto reset;
		}

		SAS_DPRINTK("trying to find task 0x%p\n", task);
		res = sas_scsi_find_task(task);

		switch (res) {
		case TASK_IS_DONE:
			SAS_DPRINTK("%s: task 0x%p is done\n", __func__,
				    task);
			sas_eh_defer_cmd(cmd);
			continue;
		case TASK_IS_ABORTED:
			SAS_DPRINTK("%s: task 0x%p is aborted\n",
				    __func__, task);
			sas_eh_defer_cmd(cmd);
			continue;
		case TASK_IS_AT_LU:
			SAS_DPRINTK("task 0x%p is at LU: lu recover\n", task);
 reset:
			tmf_resp = sas_recover_lu(task->dev, cmd);
			if (tmf_resp == TMF_RESP_FUNC_COMPLETE) {
				SAS_DPRINTK("dev %016llx LU %llx is "
					    "recovered\n",
					    SAS_ADDR(task->dev),
					    cmd->device->lun);
				sas_eh_defer_cmd(cmd);
				sas_scsi_clear_queue_lu(work_q, cmd);
				goto Again;
			}
			/* fallthrough */
		case TASK_IS_NOT_AT_LU:
		case TASK_ABORT_FAILED:
			SAS_DPRINTK("task 0x%p is not at LU: I_T recover\n",
				    task);
			tmf_resp = sas_recover_I_T(task->dev);
			if (tmf_resp == TMF_RESP_FUNC_COMPLETE ||
			    tmf_resp == -ENODEV) {
				struct domain_device *dev = task->dev;
				SAS_DPRINTK("I_T %016llx recovered\n",
					    SAS_ADDR(task->dev->sas_addr));
				sas_eh_finish_cmd(cmd);
				sas_scsi_clear_queue_I_T(work_q, dev);
				goto Again;
			}
			/* Hammer time :-) */
			try_to_reset_cmd_device(cmd);
			if (i->dft->lldd_clear_nexus_port) {
				struct asd_sas_port *port = task->dev->port;
				SAS_DPRINTK("clearing nexus for port:%d\n",
					    port->id);
				res = i->dft->lldd_clear_nexus_port(port);
				if (res == TMF_RESP_FUNC_COMPLETE) {
					SAS_DPRINTK("clear nexus port:%d "
						    "succeeded\n", port->id);
					sas_eh_finish_cmd(cmd);
					sas_scsi_clear_queue_port(work_q,
								  port);
					goto Again;
				}
			}
			if (i->dft->lldd_clear_nexus_ha) {
				SAS_DPRINTK("clear nexus ha\n");
				res = i->dft->lldd_clear_nexus_ha(ha);
				if (res == TMF_RESP_FUNC_COMPLETE) {
					SAS_DPRINTK("clear nexus ha "
						    "succeeded\n");
					sas_eh_finish_cmd(cmd);
					goto clear_q;
				}
			}
			/* If we are here -- this means that no amount
			 * of effort could recover from errors.  Quite
			 * possibly the HA just disappeared.
			 */
			SAS_DPRINTK("error from  device %llx, LUN %llx "
				    "couldn't be recovered in any way\n",
				    SAS_ADDR(task->dev->sas_addr),
				    cmd->device->lun);

			sas_eh_finish_cmd(cmd);
			goto clear_q;
		}
	}
 out:
	list_splice_tail(&done, work_q);
	list_splice_tail_init(&ha->eh_ata_q, work_q);
	return;

 clear_q:
	SAS_DPRINTK("--- Exit %s -- clear_q\n", __func__);
	list_for_each_entry_safe(cmd, n, work_q, eh_entry)
		sas_eh_finish_cmd(cmd);
	goto out;
}