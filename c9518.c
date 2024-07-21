bool bt_att_cancel(struct bt_att *att, unsigned int id)
{
	const struct queue_entry *entry;
	struct att_send_op *op;

	if (!att || !id)
		return false;

	/* Lookuo request on each channel first */
	for (entry = queue_get_entries(att->chans); entry;
						entry = entry->next) {
		struct bt_att_chan *chan = entry->data;

		if (bt_att_chan_cancel(chan, id))
			return true;
	}

	op = queue_remove_if(att->req_queue, match_op_id, UINT_TO_PTR(id));
	if (op)
		goto done;

	op = queue_remove_if(att->ind_queue, match_op_id, UINT_TO_PTR(id));
	if (op)
		goto done;

	op = queue_remove_if(att->write_queue, match_op_id, UINT_TO_PTR(id));
	if (op)
		goto done;

	if (!op)
		return false;

done:
	destroy_att_send_op(op);

	wakeup_writer(att);

	return true;
}