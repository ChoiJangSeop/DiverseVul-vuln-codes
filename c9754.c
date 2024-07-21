static int con_font_copy(struct vc_data *vc, struct console_font_op *op)
{
	int con = op->height;
	int rc;


	console_lock();
	if (vc->vc_mode != KD_TEXT)
		rc = -EINVAL;
	else if (!vc->vc_sw->con_font_copy)
		rc = -ENOSYS;
	else if (con < 0 || !vc_cons_allocated(con))
		rc = -ENOTTY;
	else if (con == vc->vc_num)	/* nothing to do */
		rc = 0;
	else
		rc = vc->vc_sw->con_font_copy(vc, con);
	console_unlock();
	return rc;
}