bind_unix_address(int sock_fd, const char *addr, int flags)
{
  union sockaddr_all saddr;

  if (snprintf(saddr.un.sun_path, sizeof (saddr.un.sun_path), "%s", addr) >=
      sizeof (saddr.un.sun_path)) {
    DEBUG_LOG("Unix socket path %s too long", addr);
    return 0;
  }
  saddr.un.sun_family = AF_UNIX;

  unlink(addr);

  /* PRV_BindSocket() doesn't support Unix sockets yet */
  if (bind(sock_fd, &saddr.sa, sizeof (saddr.un)) < 0) {
    DEBUG_LOG("Could not bind Unix socket to %s : %s", addr, strerror(errno));
    return 0;
  }

  /* Allow access to everyone with access to the directory if requested */
  if (flags & SCK_FLAG_ALL_PERMISSIONS && chmod(addr, 0666) < 0) {
    DEBUG_LOG("Could not change permissions of %s : %s", addr, strerror(errno));
    return 0;
  }

  return 1;
}