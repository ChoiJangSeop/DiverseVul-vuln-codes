h2_rx_data(struct worker *wrk, struct h2_sess *h2, struct h2_req *r2)
{
	int w1 = 0, w2 = 0;
	char buf[4];
	unsigned wi;

	CHECK_OBJ_NOTNULL(wrk, WORKER_MAGIC);
	ASSERT_RXTHR(h2);
	CHECK_OBJ_ORNULL(r2, H2_REQ_MAGIC);

	if (r2 == NULL || !r2->scheduled)
		return (0);
	if (r2->state >= H2_S_CLOS_REM) {
		r2->error = H2SE_STREAM_CLOSED;
		return (H2SE_STREAM_CLOSED); // rfc7540,l,1766,1769
	}
	Lck_Lock(&h2->sess->mtx);
	while (h2->mailcall != NULL && h2->error == 0 && r2->error == 0)
		AZ(Lck_CondWait(h2->cond, &h2->sess->mtx, 0));
	if (h2->error || r2->error) {
		Lck_Unlock(&h2->sess->mtx);
		return (h2->error ? h2->error : r2->error);
	}
	AZ(h2->mailcall);
	h2->mailcall = r2;
	h2->req0->r_window -= h2->rxf_len;
	r2->r_window -= h2->rxf_len;
	// req_bodybytes accounted in CNT code.
	if (r2->cond)
		AZ(pthread_cond_signal(r2->cond));
	while (h2->mailcall != NULL && h2->error == 0 && r2->error == 0)
		AZ(Lck_CondWait(h2->cond, &h2->sess->mtx, 0));
	wi = cache_param->h2_rx_window_increment;
	if (h2->req0->r_window < cache_param->h2_rx_window_low_water) {
		h2->req0->r_window += wi;
		w1 = 1;
	}
	if (r2->r_window < cache_param->h2_rx_window_low_water) {
		r2->r_window += wi;
		w2 = 1;
	}
	Lck_Unlock(&h2->sess->mtx);

	if (w1 || w2) {
		vbe32enc(buf, wi);
		H2_Send_Get(wrk, h2, h2->req0);
		if (w1)
			H2_Send_Frame(wrk, h2, H2_F_WINDOW_UPDATE, 0,
			    4, 0, buf);
		if (w2)
			H2_Send_Frame(wrk, h2, H2_F_WINDOW_UPDATE, 0,
			    4, r2->stream, buf);
		H2_Send_Rel(h2, h2->req0);
	}
	return (0);
}