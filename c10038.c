static int vsock_stream_setsockopt(struct socket *sock,
				   int level,
				   int optname,
				   sockptr_t optval,
				   unsigned int optlen)
{
	int err;
	struct sock *sk;
	struct vsock_sock *vsk;
	const struct vsock_transport *transport;
	u64 val;

	if (level != AF_VSOCK)
		return -ENOPROTOOPT;

#define COPY_IN(_v)                                       \
	do {						  \
		if (optlen < sizeof(_v)) {		  \
			err = -EINVAL;			  \
			goto exit;			  \
		}					  \
		if (copy_from_sockptr(&_v, optval, sizeof(_v)) != 0) {	\
			err = -EFAULT;					\
			goto exit;					\
		}							\
	} while (0)

	err = 0;
	sk = sock->sk;
	vsk = vsock_sk(sk);
	transport = vsk->transport;

	lock_sock(sk);

	switch (optname) {
	case SO_VM_SOCKETS_BUFFER_SIZE:
		COPY_IN(val);
		vsock_update_buffer_size(vsk, transport, val);
		break;

	case SO_VM_SOCKETS_BUFFER_MAX_SIZE:
		COPY_IN(val);
		vsk->buffer_max_size = val;
		vsock_update_buffer_size(vsk, transport, vsk->buffer_size);
		break;

	case SO_VM_SOCKETS_BUFFER_MIN_SIZE:
		COPY_IN(val);
		vsk->buffer_min_size = val;
		vsock_update_buffer_size(vsk, transport, vsk->buffer_size);
		break;

	case SO_VM_SOCKETS_CONNECT_TIMEOUT: {
		struct __kernel_old_timeval tv;
		COPY_IN(tv);
		if (tv.tv_sec >= 0 && tv.tv_usec < USEC_PER_SEC &&
		    tv.tv_sec < (MAX_SCHEDULE_TIMEOUT / HZ - 1)) {
			vsk->connect_timeout = tv.tv_sec * HZ +
			    DIV_ROUND_UP(tv.tv_usec, (1000000 / HZ));
			if (vsk->connect_timeout == 0)
				vsk->connect_timeout =
				    VSOCK_DEFAULT_CONNECT_TIMEOUT;

		} else {
			err = -ERANGE;
		}
		break;
	}

	default:
		err = -ENOPROTOOPT;
		break;
	}

#undef COPY_IN

exit:
	release_sock(sk);
	return err;
}