loop_info(
	sockaddr_u *srcadr,
	endpt *inter,
	struct req_pkt *inpkt
	)
{
	struct info_loop *li;
	l_fp ltmp;

	li = (struct info_loop *)prepare_pkt(srcadr, inter, inpkt,
	    sizeof(struct info_loop));

	DTOLFP(last_offset, &ltmp);
	HTONL_FP(&ltmp, &li->last_offset);
	DTOLFP(drift_comp * 1e6, &ltmp);
	HTONL_FP(&ltmp, &li->drift_comp);
	li->compliance = htonl((u_int32)(tc_counter));
	li->watchdog_timer = htonl((u_int32)(current_time - sys_epoch));

	more_pkt();
	flush_pkt();
}