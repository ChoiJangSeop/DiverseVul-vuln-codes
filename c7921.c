spool_write_header(uschar *id, int where, uschar **errmsg)
{
int fd;
int i;
int size_correction;
FILE *f;
header_line *h;
struct stat statbuf;
uschar name[256];
uschar temp_name[256];

sprintf(CS temp_name, "%s/input/%s/hdr.%d", spool_directory, message_subdir,
  (int)getpid());
fd = spool_open_temp(temp_name);
if (fd < 0) return spool_write_error(where, errmsg, US"open", NULL, NULL);
f = fdopen(fd, "wb");
DEBUG(D_receive|D_deliver) debug_printf("Writing spool header file\n");

/* We now have an open file to which the header data is to be written. Start
with the file's leaf name, to make the file self-identifying. Continue with the
identity of the submitting user, followed by the sender's address. The sender's
address is enclosed in <> because it might be the null address. Then write the
received time and the number of warning messages that have been sent. */

fprintf(f, "%s-H\n", message_id);
fprintf(f, "%.63s %ld %ld\n", originator_login, (long int)originator_uid,
  (long int)originator_gid);
fprintf(f, "<%s>\n", sender_address);
fprintf(f, "%d %d\n", received_time, warning_count);

/* If there is information about a sending host, remember it. The HELO
data can be set for local SMTP as well as remote. */

if (sender_helo_name != NULL)
  fprintf(f, "-helo_name %s\n", sender_helo_name);

if (sender_host_address != NULL)
  {
  fprintf(f, "-host_address %s.%d\n", sender_host_address, sender_host_port);
  if (sender_host_name != NULL)
    fprintf(f, "-host_name %s\n", sender_host_name);
  if (sender_host_authenticated != NULL)
    fprintf(f, "-host_auth %s\n", sender_host_authenticated);
  }

/* Also about the interface a message came in on */

if (interface_address != NULL)
  fprintf(f, "-interface_address %s.%d\n", interface_address, interface_port);

if (smtp_active_hostname != primary_hostname)
  fprintf(f, "-active_hostname %s\n", smtp_active_hostname);

/* Likewise for any ident information; for local messages this is
likely to be the same as originator_login, but will be different if
the originator was root, forcing a different ident. */

if (sender_ident != NULL) fprintf(f, "-ident %s\n", sender_ident);

/* Ditto for the received protocol */

if (received_protocol != NULL)
  fprintf(f, "-received_protocol %s\n", received_protocol);

/* Preserve any ACL variables that are set. */

tree_walk(acl_var_c, &acl_var_write, f);
tree_walk(acl_var_m, &acl_var_write, f);

/* Now any other data that needs to be remembered. */

fprintf(f, "-body_linecount %d\n", body_linecount);
fprintf(f, "-max_received_linelength %d\n", max_received_linelength);

if (body_zerocount > 0) fprintf(f, "-body_zerocount %d\n", body_zerocount);

if (authenticated_id != NULL)
  fprintf(f, "-auth_id %s\n", authenticated_id);
if (authenticated_sender != NULL)
  fprintf(f, "-auth_sender %s\n", authenticated_sender);

if (allow_unqualified_recipient) fprintf(f, "-allow_unqualified_recipient\n");
if (allow_unqualified_sender) fprintf(f, "-allow_unqualified_sender\n");
if (deliver_firsttime) fprintf(f, "-deliver_firsttime\n");
if (deliver_freeze) fprintf(f, "-frozen %d\n", deliver_frozen_at);
if (dont_deliver) fprintf(f, "-N\n");
if (host_lookup_deferred) fprintf(f, "-host_lookup_deferred\n");
if (host_lookup_failed) fprintf(f, "-host_lookup_failed\n");
if (sender_local) fprintf(f, "-local\n");
if (local_error_message) fprintf(f, "-localerror\n");
if (local_scan_data != NULL) fprintf(f, "-local_scan %s\n", local_scan_data);
#ifdef WITH_CONTENT_SCAN
if (spam_score_int != NULL) fprintf(f,"-spam_score_int %s\n", spam_score_int);
#endif
if (deliver_manual_thaw) fprintf(f, "-manual_thaw\n");
if (sender_set_untrusted) fprintf(f, "-sender_set_untrusted\n");

#ifdef EXPERIMENTAL_BRIGHTMAIL
if (bmi_verdicts != NULL) fprintf(f, "-bmi_verdicts %s\n", bmi_verdicts);
#endif

#ifdef SUPPORT_TLS
if (tls_certificate_verified) fprintf(f, "-tls_certificate_verified\n");
if (tls_cipher != NULL) fprintf(f, "-tls_cipher %s\n", tls_cipher);
if (tls_peerdn != NULL) fprintf(f, "-tls_peerdn %s\n", string_printing(tls_peerdn));
#endif

/* To complete the envelope, write out the tree of non-recipients, followed by
the list of recipients. These won't be disjoint the first time, when no
checking has been done. If a recipient is a "one-time" alias, it is followed by
a space and its parent address number (pno). */

tree_write(tree_nonrecipients, f);
fprintf(f, "%d\n", recipients_count);
for (i = 0; i < recipients_count; i++)
  {
  recipient_item *r = recipients_list + i;
  if (r->pno < 0 && r->errors_to == NULL)
    fprintf(f, "%s\n", r->address);
  else
    {
    uschar *errors_to = (r->errors_to == NULL)? US"" : r->errors_to;
    fprintf(f, "%s %s %d,%d#1\n", r->address, errors_to,
      Ustrlen(errors_to), r->pno);
    }
  }

/* Put a blank line before the headers */

fprintf(f, "\n");

/* Save the size of the file so far so we can subtract it from the final length
to get the actual size of the headers. */

fflush(f);
fstat(fd, &statbuf);
size_correction = statbuf.st_size;

/* Finally, write out the message's headers. To make it easier to read them
in again, precede each one with the count of its length. Make the count fixed
length to aid human eyes when debugging and arrange for it not be included in
the size. It is followed by a space for normal headers, a flagging letter for
various other headers, or an asterisk for old headers that have been rewritten.
These are saved as a record for debugging. Don't included them in the message's
size. */

for (h = header_list; h != NULL; h = h->next)
  {
  fprintf(f, "%03d%c %s", h->slen, h->type, h->text);
  size_correction += 5;
  if (h->type == '*') size_correction += h->slen;
  }

/* Flush and check for any errors while writing */

if (fflush(f) != 0 || ferror(f))
  return spool_write_error(where, errmsg, US"write", temp_name, f);

/* Force the file's contents to be written to disk. Note that fflush()
just pushes it out of C, and fclose() doesn't guarantee to do the write
either. That's just the way Unix works... */

if (EXIMfsync(fileno(f)) < 0)
  return spool_write_error(where, errmsg, US"sync", temp_name, f);

/* Get the size of the file, and close it. */

fstat(fd, &statbuf);
if (fclose(f) != 0)
  return spool_write_error(where, errmsg, US"close", temp_name, NULL);

/* Rename the file to its correct name, thereby replacing any previous
incarnation. */

sprintf(CS name, "%s/input/%s/%s-H", spool_directory, message_subdir, id);

if (Urename(temp_name, name) < 0)
  return spool_write_error(where, errmsg, US"rename", temp_name, NULL);

/* Linux (and maybe other OS?) does not automatically sync a directory after
an operation like rename. We therefore have to do it forcibly ourselves in
these cases, to make sure the file is actually accessible on disk, as opposed
to just the data being accessible from a file in lost+found. Linux also has
O_DIRECTORY, for opening a directory.

However, it turns out that some file systems (some versions of NFS?) do not
support directory syncing. It seems safe enough to ignore EINVAL to cope with
these cases. One hack on top of another... but that's life. */

#ifdef NEED_SYNC_DIRECTORY

sprintf(CS temp_name, "%s/input/%s/.", spool_directory, message_subdir);

#ifndef O_DIRECTORY
#define O_DIRECTORY 0
#endif

if ((fd = Uopen(temp_name, O_RDONLY|O_DIRECTORY, 0)) < 0)
  return spool_write_error(where, errmsg, US"directory open", name, NULL);

if (EXIMfsync(fd) < 0 && errno != EINVAL)
  return spool_write_error(where, errmsg, US"directory sync", name, NULL);

if (close(fd) < 0)
  return spool_write_error(where, errmsg, US"directory close", name, NULL);

#endif  /* NEED_SYNC_DIRECTORY */

/* Return the number of characters in the headers, which is the file size, less
the prelimary stuff, less the additional count fields on the headers. */

DEBUG(D_receive) debug_printf("Size of headers = %d\n",
  (int)(statbuf.st_size - size_correction));

return statbuf.st_size - size_correction;
}