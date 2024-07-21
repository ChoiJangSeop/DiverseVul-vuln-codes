bgp_attr_flags_diagnose
(
  struct peer * peer,
  const u_int8_t attr_code,
  u_int8_t desired_flags, /* how RFC says it must be */
  u_int8_t real_flags     /* on wire                 */
)
{
  u_char seen = 0, i;

  desired_flags &= ~BGP_ATTR_FLAG_EXTLEN;
  real_flags &= ~BGP_ATTR_FLAG_EXTLEN;
  for (i = 0; i <= 2; i++) /* O,T,P, but not E */
    if
    (
      CHECK_FLAG (desired_flags, attr_flag_str[i].key) !=
      CHECK_FLAG (real_flags,    attr_flag_str[i].key)
    )
    {
      zlog (peer->log, LOG_ERR, "%s attribute must%s be flagged as \"%s\"",
            LOOKUP (attr_str, attr_code),
            CHECK_FLAG (desired_flags, attr_flag_str[i].key) ? "" : " not",
            attr_flag_str[i].str);
      seen = 1;
    }
  assert (seen);
}