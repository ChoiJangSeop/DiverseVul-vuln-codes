int main(int argc, char** argv)
{
  int sck;
  int dis;
  struct sockaddr_un sa;
  size_t len;
  char* p;
  char* display;

  if (argc != 1)
  {
    printf("xrdp disconnect utility\n");
    printf("run with no parameters to disconnect you xrdp session\n");
    return 0;
  }

  display = getenv("DISPLAY");
  if (display == 0)
  {
    printf("display not set\n");
    return 1;
  }
  dis = strtol(display + 1, &p, 10);
  memset(&sa, 0, sizeof(sa));
  sa.sun_family = AF_UNIX;
  sprintf(sa.sun_path, "/tmp/xrdp_disconnect_display_%d", dis);
  if (access(sa.sun_path, F_OK) != 0)
  {
    printf("not in an xrdp session\n");
    return 1;
  }
  sck = socket(PF_UNIX, SOCK_DGRAM, 0);
  len = sizeof(sa);
  if (sendto(sck, "sig", 4, 0, (struct sockaddr*)&sa, len) > 0)
  {
    printf("message sent ok\n");
  }
  return 0;
}