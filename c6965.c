defaults (void)
{
  char *tmp;

  /* Most of the default values are 0 (and 0.0, NULL, and false).
     Just reset everything, and fill in the non-zero values.  Note
     that initializing pointers to NULL this way is technically
     illegal, but porting Wget to a machine where NULL is not all-zero
     bit pattern will be the least of the implementors' worries.  */
  xzero (opt);

  opt.cookies = true;
  opt.verbose = -1;
  opt.ntry = 20;
  opt.reclevel = 5;
  opt.add_hostdir = true;
  opt.netrc = true;
  opt.ftp_glob = true;
  opt.htmlify = true;
  opt.http_keep_alive = true;
  opt.use_proxy = true;
  opt.convert_file_only = false;
  tmp = getenv ("no_proxy");
  if (tmp)
    opt.no_proxy = sepstring (tmp);
  opt.prefer_family = prefer_none;
  opt.allow_cache = true;
  opt.if_modified_since = true;

  opt.read_timeout = 900;
  opt.use_robots = true;

  opt.remove_listing = true;

  opt.dot_bytes = 1024;
  opt.dot_spacing = 10;
  opt.dots_in_line = 50;

  opt.dns_cache = true;
  opt.ftp_pasv = true;
  /* 2014-09-07  Darshit Shah  <darnir@gmail.com>
   * opt.retr_symlinks is set to true by default. Creating symbolic links on the
   * local filesystem pose a security threat by malicious FTP Servers that
   * server a specially crafted .listing file akin to this:
   *
   * lrwxrwxrwx   1 root     root           33 Dec 25  2012 JoCxl6d8rFU -> /
   * drwxrwxr-x  15 1024     106          4096 Aug 28 02:02 JoCxl6d8rFU
   *
   * A .listing file in this fashion makes Wget susceptiple to a symlink attack
   * wherein the attacker is able to create arbitrary files, directories and
   * symbolic links on the target system and even set permissions.
   *
   * Hence, by default Wget attempts to retrieve the pointed-to files and does
   * not create the symbolic links locally.
   */
  opt.retr_symlinks = true;

#ifdef HAVE_SSL
  opt.check_cert = CHECK_CERT_ON;
  opt.ftps_resume_ssl = true;
  opt.ftps_fallback_to_ftp = false;
  opt.ftps_implicit = false;
  opt.ftps_clear_data_connection = false;
#endif

  /* The default for file name restriction defaults to the OS type. */
#if defined(WINDOWS) || defined(MSDOS) || defined(__CYGWIN__)
  opt.restrict_files_os = restrict_windows;
#elif defined(__VMS)
  opt.restrict_files_os = restrict_vms;
#else
  opt.restrict_files_os = restrict_unix;
#endif
  opt.restrict_files_ctrl = true;
  opt.restrict_files_nonascii = false;
  opt.restrict_files_case = restrict_no_case_restriction;

  opt.regex_type = regex_type_posix;

  opt.max_redirect = 20;

  opt.waitretry = 10;

#ifdef ENABLE_IRI
  opt.enable_iri = true;
#else
  opt.enable_iri = false;
#endif
  opt.locale = NULL;
  opt.encoding_remote = NULL;

  opt.useservertimestamps = true;
  opt.show_all_dns_entries = false;

  opt.warc_maxsize = 0; /* 1024 * 1024 * 1024; */
#ifdef HAVE_LIBZ
  opt.warc_compression_enabled = true;
#else
  opt.warc_compression_enabled = false;
#endif
  opt.warc_digests_enabled = true;
  opt.warc_cdx_enabled = false;
  opt.warc_cdx_dedup_filename = NULL;
  opt.warc_tempdir = NULL;
  opt.warc_keep_log = true;

  /* Use a negative value to mark the absence of --start-pos option */
  opt.start_pos = -1;
  opt.show_progress = -1;
  opt.noscroll = false;

#ifdef HAVE_HSTS
  /* HSTS is enabled by default */
  opt.hsts = true;
#endif
}