int get_random_value(unsigned char *data, int length)
{
  static const char *random_device = "/dev/urandom";
  int rv, fh, l;

  DBG2("reading %d random bytes from %s", length, random_device);
  fh = open(random_device, O_RDONLY);
  if (fh == -1) {
    set_error("open() failed: %s", strerror(errno));
    return -1;
  }

  l = 0;
  while (l < length) {
    rv = read(fh, data + l, length - l);
    if (rv <= 0) {
      close(fh);
      set_error("read() failed: %s", strerror(errno));
      return -1;
    }
    l += rv;
  }
  close(fh);
  DBG5("random-value[%d] = [%02x:%02x:%02x:...:%02x]", length, data[0],
      data[1], data[2], data[length - 1]);
  return 0;
}