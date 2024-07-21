nbd_unlocked_opt_info (struct nbd_handle *h)
{
  int err;
  nbd_completion_callback c = { .callback = go_complete, .user_data = &err };
  int r = nbd_unlocked_aio_opt_info (h, &c);

  if (r == -1)
    return r;

  r = wait_for_option (h);
  if (r == 0 && err) {
    assert (nbd_internal_is_state_negotiating (get_next_state (h)));
    set_error (err, "server replied with error to opt_info request");
    return -1;
  }
  return r;
}