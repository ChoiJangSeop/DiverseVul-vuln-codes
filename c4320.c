static int manager_dispatch_notify_fd(sd_event_source *source, int fd, uint32_t revents, void *userdata) {

        _cleanup_fdset_free_ FDSet *fds = NULL;
        Manager *m = userdata;
        char buf[NOTIFY_BUFFER_MAX+1];
        struct iovec iovec = {
                .iov_base = buf,
                .iov_len = sizeof(buf)-1,
        };
        union {
                struct cmsghdr cmsghdr;
                uint8_t buf[CMSG_SPACE(sizeof(struct ucred)) +
                            CMSG_SPACE(sizeof(int) * NOTIFY_FD_MAX)];
        } control = {};
        struct msghdr msghdr = {
                .msg_iov = &iovec,
                .msg_iovlen = 1,
                .msg_control = &control,
                .msg_controllen = sizeof(control),
        };

        struct cmsghdr *cmsg;
        struct ucred *ucred = NULL;
        bool found = false;
        Unit *u1, *u2, *u3;
        int r, *fd_array = NULL;
        unsigned n_fds = 0;
        ssize_t n;

        assert(m);
        assert(m->notify_fd == fd);

        if (revents != EPOLLIN) {
                log_warning("Got unexpected poll event for notify fd.");
                return 0;
        }

        n = recvmsg(m->notify_fd, &msghdr, MSG_DONTWAIT|MSG_CMSG_CLOEXEC);
        if (n < 0) {
                if (errno == EAGAIN || errno == EINTR)
                        return 0;

                return -errno;
        }
        if (n == 0) {
                log_debug("Got zero-length notification message. Ignoring.");
                return 0;
        }

        CMSG_FOREACH(cmsg, &msghdr) {
                if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {

                        fd_array = (int*) CMSG_DATA(cmsg);
                        n_fds = (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int);

                } else if (cmsg->cmsg_level == SOL_SOCKET &&
                           cmsg->cmsg_type == SCM_CREDENTIALS &&
                           cmsg->cmsg_len == CMSG_LEN(sizeof(struct ucred))) {

                        ucred = (struct ucred*) CMSG_DATA(cmsg);
                }
        }

        if (n_fds > 0) {
                assert(fd_array);

                r = fdset_new_array(&fds, fd_array, n_fds);
                if (r < 0) {
                        close_many(fd_array, n_fds);
                        return log_oom();
                }
        }

        if (!ucred || ucred->pid <= 0) {
                log_warning("Received notify message without valid credentials. Ignoring.");
                return 0;
        }

        if ((size_t) n >= sizeof(buf)) {
                log_warning("Received notify message exceeded maximum size. Ignoring.");
                return 0;
        }

        buf[n] = 0;

        /* Notify every unit that might be interested, but try
         * to avoid notifying the same one multiple times. */
        u1 = manager_get_unit_by_pid_cgroup(m, ucred->pid);
        if (u1) {
                manager_invoke_notify_message(m, u1, ucred->pid, buf, n, fds);
                found = true;
        }

        u2 = hashmap_get(m->watch_pids1, PID_TO_PTR(ucred->pid));
        if (u2 && u2 != u1) {
                manager_invoke_notify_message(m, u2, ucred->pid, buf, n, fds);
                found = true;
        }

        u3 = hashmap_get(m->watch_pids2, PID_TO_PTR(ucred->pid));
        if (u3 && u3 != u2 && u3 != u1) {
                manager_invoke_notify_message(m, u3, ucred->pid, buf, n, fds);
                found = true;
        }

        if (!found)
                log_warning("Cannot find unit for notify message of PID "PID_FMT".", ucred->pid);

        if (fdset_size(fds) > 0)
                log_warning("Got auxiliary fds with notification message, closing all.");

        return 0;
}