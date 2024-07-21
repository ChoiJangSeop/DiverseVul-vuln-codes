static int __socket_slurp (RSocket *s, ut8 *buf, int bufsz) {
	int i;
	int chsz = 1;
	// r_socket_block_time (s, 1, 1, 0);
	if (r_socket_read_block (s, (ut8 *) buf, 1) != 1) {
		return 0;
	}
	for (i = 1; i < bufsz; i += chsz) {
		buf[i] =0;
		r_socket_block_time (s, 1, 0, 1000);
		int olen = r_socket_read_block (s, (ut8 *) buf + i , chsz);
		if (olen != chsz) {
			buf[i] = 0;
			break;
		}
	}
	return i;
}