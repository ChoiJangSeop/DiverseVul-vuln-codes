static void rose_timer_expiry(struct timer_list *t)
{
	struct rose_sock *rose = from_timer(rose, t, timer);
	struct sock *sk = &rose->sock;

	bh_lock_sock(sk);
	switch (rose->state) {
	case ROSE_STATE_1:	/* T1 */
	case ROSE_STATE_4:	/* T2 */
		rose_write_internal(sk, ROSE_CLEAR_REQUEST);
		rose->state = ROSE_STATE_2;
		rose_start_t3timer(sk);
		break;

	case ROSE_STATE_2:	/* T3 */
		rose->neighbour->use--;
		rose_disconnect(sk, ETIMEDOUT, -1, -1);
		break;

	case ROSE_STATE_3:	/* HB */
		if (rose->condition & ROSE_COND_ACK_PENDING) {
			rose->condition &= ~ROSE_COND_ACK_PENDING;
			rose_enquiry_response(sk);
		}
		break;
	}
	bh_unlock_sock(sk);
}