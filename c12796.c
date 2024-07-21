R_API int r_socket_read_block(RSocket *s, ut8 *buf, int len) {
	int ret = 0;
	for (ret = 0; ret < len; ) {
		int r = r_socket_read (s, buf + ret, len - ret);
		if (r == -1) {
			return -1;
		}
		if (r < 1) {
			break;
		}
		ret += r;
	}
	return ret;
}