struct child_process *git_connect(int fd[2], const char *url_orig,
				  const char *prog, int flags)
{
	char *url = xstrdup(url_orig);
	char *host, *path;
	char *end;
	int c;
	struct child_process *conn;
	enum protocol protocol = PROTO_LOCAL;
	int free_path = 0;
	char *port = NULL;
	const char **arg;
	struct strbuf cmd;

	/* Without this we cannot rely on waitpid() to tell
	 * what happened to our children.
	 */
	signal(SIGCHLD, SIG_DFL);

	host = strstr(url, "://");
	if(host) {
		*host = '\0';
		protocol = get_protocol(url);
		host += 3;
		c = '/';
	} else {
		host = url;
		c = ':';
	}

	if (host[0] == '[') {
		end = strchr(host + 1, ']');
		if (end) {
			*end = 0;
			end++;
			host++;
		} else
			end = host;
	} else
		end = host;

	path = strchr(end, c);
	if (path && !has_dos_drive_prefix(end)) {
		if (c == ':') {
			protocol = PROTO_SSH;
			*path++ = '\0';
		}
	} else
		path = end;

	if (!path || !*path)
		die("No path specified. See 'man git-pull' for valid url syntax");

	/*
	 * null-terminate hostname and point path to ~ for URL's like this:
	 *    ssh://host.xz/~user/repo
	 */
	if (protocol != PROTO_LOCAL && host != url) {
		char *ptr = path;
		if (path[1] == '~')
			path++;
		else {
			path = xstrdup(ptr);
			free_path = 1;
		}

		*ptr = '\0';
	}

	/*
	 * Add support for ssh port: ssh://host.xy:<port>/...
	 */
	if (protocol == PROTO_SSH && host != url)
		port = get_port(host);

	if (protocol == PROTO_GIT) {
		/* These underlying connection commands die() if they
		 * cannot connect.
		 */
		char *target_host = xstrdup(host);
		if (git_use_proxy(host))
			git_proxy_connect(fd, host);
		else
			git_tcp_connect(fd, host, flags);
		/*
		 * Separate original protocol components prog and path
		 * from extended components with a NUL byte.
		 */
		packet_write(fd[1],
			     "%s %s%chost=%s%c",
			     prog, path, 0,
			     target_host, 0);
		free(target_host);
		free(url);
		if (free_path)
			free(path);
		return &no_fork;
	}

	conn = xcalloc(1, sizeof(*conn));

	strbuf_init(&cmd, MAX_CMD_LEN);
	strbuf_addstr(&cmd, prog);
	strbuf_addch(&cmd, ' ');
	sq_quote_buf(&cmd, path);
	if (cmd.len >= MAX_CMD_LEN)
		die("command line too long");

	conn->in = conn->out = -1;
	conn->argv = arg = xcalloc(6, sizeof(*arg));
	if (protocol == PROTO_SSH) {
		const char *ssh = getenv("GIT_SSH");
		if (!ssh) ssh = "ssh";

		*arg++ = ssh;
		if (port) {
			*arg++ = "-p";
			*arg++ = port;
		}
		*arg++ = host;
	}
	else {
		/* remove these from the environment */
		const char *env[] = {
			ALTERNATE_DB_ENVIRONMENT,
			DB_ENVIRONMENT,
			GIT_DIR_ENVIRONMENT,
			GIT_WORK_TREE_ENVIRONMENT,
			GRAFT_ENVIRONMENT,
			INDEX_ENVIRONMENT,
			NULL
		};
		conn->env = env;
		*arg++ = "sh";
		*arg++ = "-c";
	}
	*arg++ = cmd.buf;
	*arg = NULL;

	if (start_command(conn))
		die("unable to fork");

	fd[0] = conn->out; /* read from child's stdout */
	fd[1] = conn->in;  /* write to child's stdin */
	strbuf_release(&cmd);
	free(url);
	if (free_path)
		free(path);
	return conn;
}