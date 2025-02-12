static void snd_timer_check_master(struct snd_timer_instance *master)
{
	struct snd_timer_instance *slave, *tmp;

	/* check all pending slaves */
	list_for_each_entry_safe(slave, tmp, &snd_timer_slave_list, open_list) {
		if (slave->slave_class == master->slave_class &&
		    slave->slave_id == master->slave_id) {
			list_move_tail(&slave->open_list, &master->slave_list_head);
			spin_lock_irq(&slave_active_lock);
			slave->master = master;
			slave->timer = master->timer;
			if (slave->flags & SNDRV_TIMER_IFLG_RUNNING)
				list_add_tail(&slave->active_list,
					      &master->slave_active_head);
			spin_unlock_irq(&slave_active_lock);
		}
	}
}