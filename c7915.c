spool_read_header(uschar *name, BOOL read_headers, BOOL subdir_set)
{
FILE *f = NULL;
int n;
int rcount = 0;
long int uid, gid;
BOOL inheader = FALSE;
uschar *p;

/* Reset all the global variables to their default values. However, there is
one exception. DO NOT change the default value of dont_deliver, because it may
be forced by an external setting. */

acl_var_c = acl_var_m = NULL;
authenticated_id = NULL;
authenticated_sender = NULL;
allow_unqualified_recipient = FALSE;
allow_unqualified_sender = FALSE;
body_linecount = 0;
body_zerocount = 0;
deliver_firsttime = FALSE;
deliver_freeze = FALSE;
deliver_frozen_at = 0;
deliver_manual_thaw = FALSE;
/* dont_deliver must NOT be reset */
header_list = header_last = NULL;
host_lookup_deferred = FALSE;
host_lookup_failed = FALSE;
interface_address = NULL;
interface_port = 0;
local_error_message = FALSE;
local_scan_data = NULL;
max_received_linelength = 0;
message_linecount = 0;
received_protocol = NULL;
received_count = 0;
recipients_list = NULL;
sender_address = NULL;
sender_fullhost = NULL;
sender_helo_name = NULL;
sender_host_address = NULL;
sender_host_name = NULL;
sender_host_port = 0;
sender_host_authenticated = NULL;
sender_ident = NULL;
sender_local = FALSE;
sender_set_untrusted = FALSE;
smtp_active_hostname = primary_hostname;
tree_nonrecipients = NULL;

#ifdef EXPERIMENTAL_BRIGHTMAIL
bmi_run = 0;
bmi_verdicts = NULL;
#endif

#ifndef DISABLE_DKIM
dkim_signers = NULL;
dkim_disable_verify = FALSE;
dkim_collect_input = FALSE;
#endif

#ifdef SUPPORT_TLS
tls_certificate_verified = FALSE;
tls_cipher = NULL;
tls_peerdn = NULL;
#endif

#ifdef WITH_CONTENT_SCAN
spam_score_int = NULL;
#endif

/* Generate the full name and open the file. If message_subdir is already
set, just look in the given directory. Otherwise, look in both the split
and unsplit directories, as for the data file above. */

for (n = 0; n < 2; n++)
  {
  if (!subdir_set)
    message_subdir[0] = (split_spool_directory == (n == 0))? name[5] : 0;
  sprintf(CS big_buffer, "%s/input/%s/%s", spool_directory, message_subdir,
    name);
  f = Ufopen(big_buffer, "rb");
  if (f != NULL) break;
  if (n != 0 || subdir_set || errno != ENOENT) return spool_read_notopen;
  }

errno = 0;

#ifndef COMPILE_UTILITY
DEBUG(D_deliver) debug_printf("reading spool file %s\n", name);
#endif  /* COMPILE_UTILITY */

/* The first line of a spool file contains the message id followed by -H (i.e.
the file name), in order to make the file self-identifying. */

if (Ufgets(big_buffer, big_buffer_size, f) == NULL) goto SPOOL_READ_ERROR;
if (Ustrlen(big_buffer) != MESSAGE_ID_LENGTH + 3 ||
    Ustrncmp(big_buffer, name, MESSAGE_ID_LENGTH + 2) != 0)
  goto SPOOL_FORMAT_ERROR;

/* The next three lines in the header file are in a fixed format. The first
contains the login, uid, and gid of the user who caused the file to be written.
There are known cases where a negative gid is used, so we allow for both
negative uids and gids. The second contains the mail address of the message's
sender, enclosed in <>. The third contains the time the message was received,
and the number of warning messages for delivery delays that have been sent. */

if (Ufgets(big_buffer, big_buffer_size, f) == NULL) goto SPOOL_READ_ERROR;

p = big_buffer + Ustrlen(big_buffer);
while (p > big_buffer && isspace(p[-1])) p--;
*p = 0;
if (!isdigit(p[-1])) goto SPOOL_FORMAT_ERROR;
while (p > big_buffer && (isdigit(p[-1]) || '-' == p[-1])) p--;
gid = Uatoi(p);
if (p <= big_buffer || *(--p) != ' ') goto SPOOL_FORMAT_ERROR;
*p = 0;
if (!isdigit(p[-1])) goto SPOOL_FORMAT_ERROR;
while (p > big_buffer && (isdigit(p[-1]) || '-' == p[-1])) p--;
uid = Uatoi(p);
if (p <= big_buffer || *(--p) != ' ') goto SPOOL_FORMAT_ERROR;
*p = 0;

originator_login = string_copy(big_buffer);
originator_uid = (uid_t)uid;
originator_gid = (gid_t)gid;

if (Ufgets(big_buffer, big_buffer_size, f) == NULL) goto SPOOL_READ_ERROR;
n = Ustrlen(big_buffer);
if (n < 3 || big_buffer[0] != '<' || big_buffer[n-2] != '>')
  goto SPOOL_FORMAT_ERROR;

sender_address = store_get(n-2);
Ustrncpy(sender_address, big_buffer+1, n-3);
sender_address[n-3] = 0;

if (Ufgets(big_buffer, big_buffer_size, f) == NULL) goto SPOOL_READ_ERROR;
if (sscanf(CS big_buffer, "%d %d", &received_time, &warning_count) != 2)
  goto SPOOL_FORMAT_ERROR;

message_age = time(NULL) - received_time;

#ifndef COMPILE_UTILITY
DEBUG(D_deliver) debug_printf("user=%s uid=%ld gid=%ld sender=%s\n",
  originator_login, (long int)originator_uid, (long int)originator_gid,
  sender_address);
#endif  /* COMPILE_UTILITY */

/* Now there may be a number of optional lines, each starting with "-". If you
add a new setting here, make sure you set the default above.

Because there are now quite a number of different possibilities, we use a
switch on the first character to avoid too many failing tests. Thanks to Nico
Erfurth for the patch that implemented this. I have made it even more efficient
by not re-scanning the first two characters.

To allow new versions of Exim that add additional flags to interwork with older
versions that do not understand them, just ignore any lines starting with "-"
that we don't recognize. Otherwise it wouldn't be possible to back off a new
version that left new-style flags written on the spool. */

p = big_buffer + 2;
for (;;)
  {
  if (Ufgets(big_buffer, big_buffer_size, f) == NULL) goto SPOOL_READ_ERROR;
  if (big_buffer[0] != '-') break;
  big_buffer[Ustrlen(big_buffer) - 1] = 0;

  switch(big_buffer[1])
    {
    case 'a':

    /* Nowadays we use "-aclc" and "-aclm" for the different types of ACL
    variable, because Exim allows any number of them, with arbitrary names.
    The line in the spool file is "-acl[cm] <name> <length>". The name excludes
    the c or m. */

    if (Ustrncmp(p, "clc ", 4) == 0 ||
        Ustrncmp(p, "clm ", 4) == 0)
      {
      uschar *name, *endptr;
      int count;
      tree_node *node;
      endptr = Ustrchr(big_buffer + 6, ' ');
      if (endptr == NULL) goto SPOOL_FORMAT_ERROR;
      name = string_sprintf("%c%.*s", big_buffer[4], endptr - big_buffer - 6,
        big_buffer + 6);
      if (sscanf(CS endptr, " %d", &count) != 1) goto SPOOL_FORMAT_ERROR;
      node = acl_var_create(name);
      node->data.ptr = store_get(count + 1);
      if (fread(node->data.ptr, 1, count+1, f) < count) goto SPOOL_READ_ERROR;
      ((uschar*)node->data.ptr)[count] = 0;
      }

    else if (Ustrcmp(p, "llow_unqualified_recipient") == 0)
      allow_unqualified_recipient = TRUE;
    else if (Ustrcmp(p, "llow_unqualified_sender") == 0)
      allow_unqualified_sender = TRUE;

    else if (Ustrncmp(p, "uth_id", 6) == 0)
      authenticated_id = string_copy(big_buffer + 9);
    else if (Ustrncmp(p, "uth_sender", 10) == 0)
      authenticated_sender = string_copy(big_buffer + 13);
    else if (Ustrncmp(p, "ctive_hostname", 14) == 0)
      smtp_active_hostname = string_copy(big_buffer + 17);

    /* For long-term backward compatibility, we recognize "-acl", which was
    used before the number of ACL variables changed from 10 to 20. This was
    before the subsequent change to an arbitrary number of named variables.
    This code is retained so that upgrades from very old versions can still
    handle old-format spool files. The value given after "-acl" is a number
    that is 0-9 for connection variables, and 10-19 for message variables. */

    else if (Ustrncmp(p, "cl ", 3) == 0)
      {
      int index, count;
      uschar name[20];   /* Need plenty of space for %d format */
      tree_node *node;
      if (sscanf(CS big_buffer + 5, "%d %d", &index, &count) != 2)
        goto SPOOL_FORMAT_ERROR;
      if (index < 10)
        (void) string_format(name, sizeof(name), "%c%d", 'c', index);
      else if (index < 20) /* ignore out-of-range index */
        (void) string_format(name, sizeof(name), "%c%d", 'm', index - 10);
      node = acl_var_create(name);
      node->data.ptr = store_get(count + 1);
      if (fread(node->data.ptr, 1, count+1, f) < count) goto SPOOL_READ_ERROR;
      ((uschar*)node->data.ptr)[count] = 0;
      }
    break;

    case 'b':
    if (Ustrncmp(p, "ody_linecount", 13) == 0)
      body_linecount = Uatoi(big_buffer + 15);
    else if (Ustrncmp(p, "ody_zerocount", 13) == 0)
      body_zerocount = Uatoi(big_buffer + 15);
    #ifdef EXPERIMENTAL_BRIGHTMAIL
    else if (Ustrncmp(p, "mi_verdicts ", 12) == 0)
      bmi_verdicts = string_copy(big_buffer + 14);
    #endif
    break;

    case 'd':
    if (Ustrcmp(p, "eliver_firsttime") == 0)
      deliver_firsttime = TRUE;
    break;

    case 'f':
    if (Ustrncmp(p, "rozen", 5) == 0)
      {
      deliver_freeze = TRUE;
      deliver_frozen_at = Uatoi(big_buffer + 7);
      }
    break;

    case 'h':
    if (Ustrcmp(p, "ost_lookup_deferred") == 0)
      host_lookup_deferred = TRUE;
    else if (Ustrcmp(p, "ost_lookup_failed") == 0)
      host_lookup_failed = TRUE;
    else if (Ustrncmp(p, "ost_auth", 8) == 0)
      sender_host_authenticated = string_copy(big_buffer + 11);
    else if (Ustrncmp(p, "ost_name", 8) == 0)
      sender_host_name = string_copy(big_buffer + 11);
    else if (Ustrncmp(p, "elo_name", 8) == 0)
      sender_helo_name = string_copy(big_buffer + 11);

    /* We now record the port number after the address, separated by a
    dot. For compatibility during upgrading, do nothing if there
    isn't a value (it gets left at zero). */

    else if (Ustrncmp(p, "ost_address", 11) == 0)
      {
      sender_host_port = host_address_extract_port(big_buffer + 14);
      sender_host_address = string_copy(big_buffer + 14);
      }
    break;

    case 'i':
    if (Ustrncmp(p, "nterface_address", 16) == 0)
      {
      interface_port = host_address_extract_port(big_buffer + 19);
      interface_address = string_copy(big_buffer + 19);
      }
    else if (Ustrncmp(p, "dent", 4) == 0)
      sender_ident = string_copy(big_buffer + 7);
    break;

    case 'l':
    if (Ustrcmp(p, "ocal") == 0) sender_local = TRUE;
    else if (Ustrcmp(big_buffer, "-localerror") == 0)
      local_error_message = TRUE;
    else if (Ustrncmp(p, "ocal_scan ", 10) == 0)
      local_scan_data = string_copy(big_buffer + 12);
    break;

    case 'm':
    if (Ustrcmp(p, "anual_thaw") == 0) deliver_manual_thaw = TRUE;
    else if (Ustrncmp(p, "ax_received_linelength", 22) == 0)
      max_received_linelength = Uatoi(big_buffer + 24);
    break;

    case 'N':
    if (*p == 0) dont_deliver = TRUE;   /* -N */
    break;

    case 'r':
    if (Ustrncmp(p, "eceived_protocol", 16) == 0)
      received_protocol = string_copy(big_buffer + 19);
    break;

    case 's':
    if (Ustrncmp(p, "ender_set_untrusted", 19) == 0)
      sender_set_untrusted = TRUE;
    #ifdef WITH_CONTENT_SCAN
    else if (Ustrncmp(p, "pam_score_int ", 14) == 0)
      spam_score_int = string_copy(big_buffer + 16);
    #endif
    break;

    #ifdef SUPPORT_TLS
    case 't':
    if (Ustrncmp(p, "ls_certificate_verified", 23) == 0)
      tls_certificate_verified = TRUE;
    else if (Ustrncmp(p, "ls_cipher", 9) == 0)
      tls_cipher = string_copy(big_buffer + 12);
    else if (Ustrncmp(p, "ls_peerdn", 9) == 0)
      tls_peerdn = string_unprinting(string_copy(big_buffer + 12));
    break;
    #endif

    default:    /* Present because some compilers complain if all */
    break;      /* possibilities are not covered. */
    }
  }

/* Build sender_fullhost if required */

#ifndef COMPILE_UTILITY
host_build_sender_fullhost();
#endif  /* COMPILE_UTILITY */

#ifndef COMPILE_UTILITY
DEBUG(D_deliver)
  debug_printf("sender_local=%d ident=%s\n", sender_local,
    (sender_ident == NULL)? US"unset" : sender_ident);
#endif  /* COMPILE_UTILITY */

/* We now have the tree of addresses NOT to deliver to, or a line
containing "XX", indicating no tree. */

if (Ustrncmp(big_buffer, "XX\n", 3) != 0 &&
  !read_nonrecipients_tree(&tree_nonrecipients, f, big_buffer, big_buffer_size))
    goto SPOOL_FORMAT_ERROR;

#ifndef COMPILE_UTILITY
DEBUG(D_deliver)
  {
  debug_printf("Non-recipients:\n");
  debug_print_tree(tree_nonrecipients);
  }
#endif  /* COMPILE_UTILITY */

/* After reading the tree, the next line has not yet been read into the
buffer. It contains the count of recipients which follow on separate lines. */

if (Ufgets(big_buffer, big_buffer_size, f) == NULL) goto SPOOL_READ_ERROR;
if (sscanf(CS big_buffer, "%d", &rcount) != 1) goto SPOOL_FORMAT_ERROR;

#ifndef COMPILE_UTILITY
DEBUG(D_deliver) debug_printf("recipients_count=%d\n", rcount);
#endif  /* COMPILE_UTILITY */

recipients_list_max = rcount;
recipients_list = store_get(rcount * sizeof(recipient_item));

for (recipients_count = 0; recipients_count < rcount; recipients_count++)
  {
  int nn;
  int pno = -1;
  uschar *errors_to = NULL;
  uschar *p;

  if (Ufgets(big_buffer, big_buffer_size, f) == NULL) goto SPOOL_READ_ERROR;
  nn = Ustrlen(big_buffer);
  if (nn < 2) goto SPOOL_FORMAT_ERROR;

  /* Remove the newline; this terminates the address if there is no additional
  data on the line. */

  p = big_buffer + nn - 1;
  *p-- = 0;

  /* Look back from the end of the line for digits and special terminators.
  Since an address must end with a domain, we can tell that extra data is
  present by the presence of the terminator, which is always some character
  that cannot exist in a domain. (If I'd thought of the need for additional
  data early on, I'd have put it at the start, with the address at the end. As
  it is, we have to operate backwards. Addresses are permitted to contain
  spaces, you see.)

  This code has to cope with various versions of this data that have evolved
  over time. In all cases, the line might just contain an address, with no
  additional data. Otherwise, the possibilities are as follows:

  Exim 3 type:       <address><space><digits>,<digits>,<digits>

    The second set of digits is the parent number for one_time addresses. The
    other values were remnants of earlier experiments that were abandoned.

  Exim 4 first type: <address><space><digits>

    The digits are the parent number for one_time addresses.

  Exim 4 new type:   <address><space><data>#<type bits>

    The type bits indicate what the contents of the data are.

    Bit 01 indicates that, reading from right to left, the data
      ends with <errors_to address><space><len>,<pno> where pno is
      the parent number for one_time addresses, and len is the length
      of the errors_to address (zero meaning none).
   */

  while (isdigit(*p)) p--;

  /* Handle Exim 3 spool files */

  if (*p == ',')
    {
    int dummy;
    while (isdigit(*(--p)) || *p == ',');
    if (*p == ' ')
      {
      *p++ = 0;
      (void)sscanf(CS p, "%d,%d", &dummy, &pno);
      }
    }

  /* Handle early Exim 4 spool files */

  else if (*p == ' ')
    {
    *p++ = 0;
    (void)sscanf(CS p, "%d", &pno);
    }

  /* Handle current format Exim 4 spool files */

  else if (*p == '#')
    {
    int flags;
    (void)sscanf(CS p+1, "%d", &flags);

    if ((flags & 0x01) != 0)      /* one_time data exists */
      {
      int len;
      while (isdigit(*(--p)) || *p == ',' || *p == '-');
      (void)sscanf(CS p+1, "%d,%d", &len, &pno);
      *p = 0;
      if (len > 0)
        {
        p -= len;
        errors_to = string_copy(p);
        }
      }

    *(--p) = 0;   /* Terminate address */
    }

  recipients_list[recipients_count].address = string_copy(big_buffer);
  recipients_list[recipients_count].pno = pno;
  recipients_list[recipients_count].errors_to = errors_to;
  }

/* The remainder of the spool header file contains the headers for the message,
separated off from the previous data by a blank line. Each header is preceded
by a count of its length and either a certain letter (for various identified
headers), space (for a miscellaneous live header) or an asterisk (for a header
that has been rewritten). Count the Received: headers. We read the headers
always, in order to check on the format of the file, but only create a header
list if requested to do so. */

inheader = TRUE;
if (Ufgets(big_buffer, big_buffer_size, f) == NULL) goto SPOOL_READ_ERROR;
if (big_buffer[0] != '\n') goto SPOOL_FORMAT_ERROR;

while ((n = fgetc(f)) != EOF)
  {
  header_line *h;
  uschar flag[4];
  int i;

  if (!isdigit(n)) goto SPOOL_FORMAT_ERROR;
  (void)ungetc(n, f);
  (void)fscanf(f, "%d%c ", &n, flag);
  if (flag[0] != '*') message_size += n;  /* Omit non-transmitted headers */

  if (read_headers)
    {
    h = store_get(sizeof(header_line));
    h->next = NULL;
    h->type = flag[0];
    h->slen = n;
    h->text = store_get(n+1);

    if (h->type == htype_received) received_count++;

    if (header_list == NULL) header_list = h;
      else header_last->next = h;
    header_last = h;

    for (i = 0; i < n; i++)
      {
      int c = fgetc(f);
      if (c == 0 || c == EOF) goto SPOOL_FORMAT_ERROR;
      if (c == '\n' && h->type != htype_old) message_linecount++;
      h->text[i] = c;
      }
    h->text[i] = 0;
    }

  /* Not requiring header data, just skip through the bytes */

  else for (i = 0; i < n; i++)
    {
    int c = fgetc(f);
    if (c == 0 || c == EOF) goto SPOOL_FORMAT_ERROR;
    }
  }

/* We have successfully read the data in the header file. Update the message
line count by adding the body linecount to the header linecount. Close the file
and give a positive response. */

#ifndef COMPILE_UTILITY
DEBUG(D_deliver) debug_printf("body_linecount=%d message_linecount=%d\n",
  body_linecount, message_linecount);
#endif  /* COMPILE_UTILITY */

message_linecount += body_linecount;

fclose(f);
return spool_read_OK;


/* There was an error reading the spool or there was missing data,
or there was a format error. A "read error" with no errno means an
unexpected EOF, which we treat as a format error. */

SPOOL_READ_ERROR:
if (errno != 0)
  {
  n = errno;

  #ifndef COMPILE_UTILITY
  DEBUG(D_any) debug_printf("Error while reading spool file %s\n", name);
  #endif  /* COMPILE_UTILITY */

  fclose(f);
  errno = n;
  return inheader? spool_read_hdrerror : spool_read_enverror;
  }

SPOOL_FORMAT_ERROR:

#ifndef COMPILE_UTILITY
DEBUG(D_any) debug_printf("Format error in spool file %s\n", name);
#endif  /* COMPILE_UTILITY */

fclose(f);
errno = ERRNO_SPOOLFORMAT;
return inheader? spool_read_hdrerror : spool_read_enverror;
}