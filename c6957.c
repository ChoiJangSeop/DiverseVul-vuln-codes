print_help (void)
{
#ifndef TESTING
  /* We split the help text this way to ease translation of individual
     entries.  */
  static const char *help[] = {
    "\n",
    N_("\
Mandatory arguments to long options are mandatory for short options too.\n\n"),
    N_("\
Startup:\n"),
    N_("\
  -V,  --version                   display the version of Wget and exit\n"),
    N_("\
  -h,  --help                      print this help\n"),
    N_("\
  -b,  --background                go to background after startup\n"),
    N_("\
  -e,  --execute=COMMAND           execute a `.wgetrc'-style command\n"),
    "\n",

    N_("\
Logging and input file:\n"),
    N_("\
  -o,  --output-file=FILE          log messages to FILE\n"),
    N_("\
  -a,  --append-output=FILE        append messages to FILE\n"),
#ifdef ENABLE_DEBUG
    N_("\
  -d,  --debug                     print lots of debugging information\n"),
#endif
#ifdef USE_WATT32
    N_("\
       --wdebug                    print Watt-32 debug output\n"),
#endif
    N_("\
  -q,  --quiet                     quiet (no output)\n"),
    N_("\
  -v,  --verbose                   be verbose (this is the default)\n"),
    N_("\
  -nv, --no-verbose                turn off verboseness, without being quiet\n"),
    N_("\
       --report-speed=TYPE         output bandwidth as TYPE.  TYPE can be bits\n"),
    N_("\
  -i,  --input-file=FILE           download URLs found in local or external FILE\n"),
#ifdef HAVE_METALINK
    N_("\
       --input-metalink=FILE       download files covered in local Metalink FILE\n"),
#endif
    N_("\
  -F,  --force-html                treat input file as HTML\n"),
    N_("\
  -B,  --base=URL                  resolves HTML input-file links (-i -F)\n\
                                     relative to URL\n"),
    N_("\
       --config=FILE               specify config file to use\n"),
    N_("\
       --no-config                 do not read any config file\n"),
    N_("\
       --rejected-log=FILE         log reasons for URL rejection to FILE\n"),
    "\n",

    N_("\
Download:\n"),
    N_("\
  -t,  --tries=NUMBER              set number of retries to NUMBER (0 unlimits)\n"),
    N_("\
       --retry-connrefused         retry even if connection is refused\n"),
    N_("\
       --retry-on-http-error=ERRORS    comma-separated list of HTTP errors to retry\n"),
    N_("\
  -O,  --output-document=FILE      write documents to FILE\n"),
    N_("\
  -nc, --no-clobber                skip downloads that would download to\n\
                                     existing files (overwriting them)\n"),
    N_("\
       --no-netrc                  don't try to obtain credentials from .netrc\n"),
    N_("\
  -c,  --continue                  resume getting a partially-downloaded file\n"),
    N_("\
       --start-pos=OFFSET          start downloading from zero-based position OFFSET\n"),
    N_("\
       --progress=TYPE             select progress gauge type\n"),
    N_("\
       --show-progress             display the progress bar in any verbosity mode\n"),
    N_("\
  -N,  --timestamping              don't re-retrieve files unless newer than\n\
                                     local\n"),
    N_("\
       --no-if-modified-since      don't use conditional if-modified-since get\n\
                                     requests in timestamping mode\n"),
    N_("\
       --no-use-server-timestamps  don't set the local file's timestamp by\n\
                                     the one on the server\n"),
    N_("\
  -S,  --server-response           print server response\n"),
    N_("\
       --spider                    don't download anything\n"),
    N_("\
  -T,  --timeout=SECONDS           set all timeout values to SECONDS\n"),
#ifdef HAVE_LIBCARES
    N_("\
       --dns-servers=ADDRESSES     list of DNS servers to query (comma separated)\n"),
    N_("\
       --bind-dns-address=ADDRESS  bind DNS resolver to ADDRESS (hostname or IP) on local host\n"),
#endif
    N_("\
       --dns-timeout=SECS          set the DNS lookup timeout to SECS\n"),
    N_("\
       --connect-timeout=SECS      set the connect timeout to SECS\n"),
    N_("\
       --read-timeout=SECS         set the read timeout to SECS\n"),
    N_("\
  -w,  --wait=SECONDS              wait SECONDS between retrievals\n"),
    N_("\
       --waitretry=SECONDS         wait 1..SECONDS between retries of a retrieval\n"),
    N_("\
       --random-wait               wait from 0.5*WAIT...1.5*WAIT secs between retrievals\n"),
    N_("\
       --no-proxy                  explicitly turn off proxy\n"),
    N_("\
  -Q,  --quota=NUMBER              set retrieval quota to NUMBER\n"),
    N_("\
       --bind-address=ADDRESS      bind to ADDRESS (hostname or IP) on local host\n"),
    N_("\
       --limit-rate=RATE           limit download rate to RATE\n"),
    N_("\
       --no-dns-cache              disable caching DNS lookups\n"),
    N_("\
       --restrict-file-names=OS    restrict chars in file names to ones OS allows\n"),
    N_("\
       --ignore-case               ignore case when matching files/directories\n"),
#ifdef ENABLE_IPV6
    N_("\
  -4,  --inet4-only                connect only to IPv4 addresses\n"),
    N_("\
  -6,  --inet6-only                connect only to IPv6 addresses\n"),
    N_("\
       --prefer-family=FAMILY      connect first to addresses of specified family,\n\
                                     one of IPv6, IPv4, or none\n"),
#endif
    N_("\
       --user=USER                 set both ftp and http user to USER\n"),
    N_("\
       --password=PASS             set both ftp and http password to PASS\n"),
    N_("\
       --ask-password              prompt for passwords\n"),
    N_("\
       --use-askpass=COMMAND       specify credential handler for requesting \n\
                                     username and password.  If no COMMAND is \n\
                                     specified the WGET_ASKPASS or the SSH_ASKPASS \n\
                                     environment variable is used.\n"),
    N_("\
       --no-iri                    turn off IRI support\n"),
    N_("\
       --local-encoding=ENC        use ENC as the local encoding for IRIs\n"),
    N_("\
       --remote-encoding=ENC       use ENC as the default remote encoding\n"),
    N_("\
       --unlink                    remove file before clobber\n"),
#ifdef HAVE_METALINK
    N_("\
       --keep-badhash              keep files with checksum mismatch (append .badhash)\n"),
    N_("\
       --metalink-index=NUMBER     Metalink application/metalink4+xml metaurl ordinal NUMBER\n"),
    N_("\
       --metalink-over-http        use Metalink metadata from HTTP response headers\n"),
    N_("\
       --preferred-location        preferred location for Metalink resources\n"),
#endif
#ifdef ENABLE_XATTR
    N_("\
       --no-xattr                  turn off storage of metadata in extended file attributes\n"),
#endif
    "\n",

    N_("\
Directories:\n"),
    N_("\
  -nd, --no-directories            don't create directories\n"),
    N_("\
  -x,  --force-directories         force creation of directories\n"),
    N_("\
  -nH, --no-host-directories       don't create host directories\n"),
    N_("\
       --protocol-directories      use protocol name in directories\n"),
    N_("\
  -P,  --directory-prefix=PREFIX   save files to PREFIX/..\n"),
    N_("\
       --cut-dirs=NUMBER           ignore NUMBER remote directory components\n"),
    "\n",

    N_("\
HTTP options:\n"),
    N_("\
       --http-user=USER            set http user to USER\n"),
    N_("\
       --http-password=PASS        set http password to PASS\n"),
    N_("\
       --no-cache                  disallow server-cached data\n"),
    N_ ("\
       --default-page=NAME         change the default page name (normally\n\
                                     this is 'index.html'.)\n"),
    N_("\
  -E,  --adjust-extension          save HTML/CSS documents with proper extensions\n"),
    N_("\
       --ignore-length             ignore 'Content-Length' header field\n"),
    N_("\
       --header=STRING             insert STRING among the headers\n"),
#ifdef HAVE_LIBZ
    N_("\
       --compression=TYPE          choose compression, one of auto, gzip and none. (default: none)\n"),
#endif
    N_("\
       --max-redirect              maximum redirections allowed per page\n"),
    N_("\
       --proxy-user=USER           set USER as proxy username\n"),
    N_("\
       --proxy-password=PASS       set PASS as proxy password\n"),
    N_("\
       --referer=URL               include 'Referer: URL' header in HTTP request\n"),
    N_("\
       --save-headers              save the HTTP headers to file\n"),
    N_("\
  -U,  --user-agent=AGENT          identify as AGENT instead of Wget/VERSION\n"),
    N_("\
       --no-http-keep-alive        disable HTTP keep-alive (persistent connections)\n"),
    N_("\
       --no-cookies                don't use cookies\n"),
    N_("\
       --load-cookies=FILE         load cookies from FILE before session\n"),
    N_("\
       --save-cookies=FILE         save cookies to FILE after session\n"),
    N_("\
       --keep-session-cookies      load and save session (non-permanent) cookies\n"),
    N_("\
       --post-data=STRING          use the POST method; send STRING as the data\n"),
    N_("\
       --post-file=FILE            use the POST method; send contents of FILE\n"),
    N_("\
       --method=HTTPMethod         use method \"HTTPMethod\" in the request\n"),
    N_("\
       --body-data=STRING          send STRING as data. --method MUST be set\n"),
    N_("\
       --body-file=FILE            send contents of FILE. --method MUST be set\n"),
    N_("\
       --content-disposition       honor the Content-Disposition header when\n\
                                     choosing local file names (EXPERIMENTAL)\n"),
    N_("\
       --content-on-error          output the received content on server errors\n"),
    N_("\
       --auth-no-challenge         send Basic HTTP authentication information\n\
                                     without first waiting for the server's\n\
                                     challenge\n"),
    "\n",

#ifdef HAVE_SSL
    N_("\
HTTPS (SSL/TLS) options:\n"),
    N_("\
       --secure-protocol=PR        choose secure protocol, one of auto, SSLv2,\n\
                                     SSLv3, TLSv1, TLSv1_1, TLSv1_2 and PFS\n"),
    N_("\
       --https-only                only follow secure HTTPS links\n"),
    N_("\
       --no-check-certificate      don't validate the server's certificate\n"),
    N_("\
       --certificate=FILE          client certificate file\n"),
    N_("\
       --certificate-type=TYPE     client certificate type, PEM or DER\n"),
    N_("\
       --private-key=FILE          private key file\n"),
    N_("\
       --private-key-type=TYPE     private key type, PEM or DER\n"),
    N_("\
       --ca-certificate=FILE       file with the bundle of CAs\n"),
    N_("\
       --ca-directory=DIR          directory where hash list of CAs is stored\n"),
    N_("\
       --crl-file=FILE             file with bundle of CRLs\n"),
    N_("\
       --pinnedpubkey=FILE/HASHES  Public key (PEM/DER) file, or any number\n\
                                   of base64 encoded sha256 hashes preceded by\n\
                                   \'sha256//\' and separated by \';\', to verify\n\
                                   peer against\n"),
#if defined(HAVE_LIBSSL) || defined(HAVE_LIBSSL32)
    N_("\
       --random-file=FILE          file with random data for seeding the SSL PRNG\n"),
#endif
#if (defined(HAVE_LIBSSL) || defined(HAVE_LIBSSL32)) && defined(HAVE_RAND_EGD)
    N_("\
       --egd-file=FILE             file naming the EGD socket with random data\n"),
#endif
    "\n",
    N_("\
       --ciphers=STR           Set the priority string (GnuTLS) or cipher list string (OpenSSL) directly.\n\
                                   Use with care. This option overrides --secure-protocol.\n\
                                   The format and syntax of this string depend on the specific SSL/TLS engine.\n"),
#endif /* HAVE_SSL */

#ifdef HAVE_HSTS
    N_("\
HSTS options:\n"),
    N_("\
       --no-hsts                   disable HSTS\n"),
    N_("\
       --hsts-file                 path of HSTS database (will override default)\n"),
    "\n",
#endif

    N_("\
FTP options:\n"),
#ifdef __VMS
    N_("\
       --ftp-stmlf                 use Stream_LF format for all binary FTP files\n"),
#endif /* def __VMS */
    N_("\
       --ftp-user=USER             set ftp user to USER\n"),
    N_("\
       --ftp-password=PASS         set ftp password to PASS\n"),
    N_("\
       --no-remove-listing         don't remove '.listing' files\n"),
    N_("\
       --no-glob                   turn off FTP file name globbing\n"),
    N_("\
       --no-passive-ftp            disable the \"passive\" transfer mode\n"),
    N_("\
       --preserve-permissions      preserve remote file permissions\n"),
    N_("\
       --retr-symlinks             when recursing, get linked-to files (not dir)\n"),
    "\n",

#ifdef HAVE_SSL
    N_("\
FTPS options:\n"),
    N_("\
       --ftps-implicit                 use implicit FTPS (default port is 990)\n"),
    N_("\
       --ftps-resume-ssl               resume the SSL/TLS session started in the control connection when\n"
        "                                         opening a data connection\n"),
    N_("\
       --ftps-clear-data-connection    cipher the control channel only; all the data will be in plaintext\n"),
    N_("\
       --ftps-fallback-to-ftp          fall back to FTP if FTPS is not supported in the target server\n"),
#endif

    N_("\
WARC options:\n"),
    N_("\
       --warc-file=FILENAME        save request/response data to a .warc.gz file\n"),
    N_("\
       --warc-header=STRING        insert STRING into the warcinfo record\n"),
    N_("\
       --warc-max-size=NUMBER      set maximum size of WARC files to NUMBER\n"),
    N_("\
       --warc-cdx                  write CDX index files\n"),
    N_("\
       --warc-dedup=FILENAME       do not store records listed in this CDX file\n"),
#ifdef HAVE_LIBZ
    N_("\
       --no-warc-compression       do not compress WARC files with GZIP\n"),
#endif
    N_("\
       --no-warc-digests           do not calculate SHA1 digests\n"),
    N_("\
       --no-warc-keep-log          do not store the log file in a WARC record\n"),
    N_("\
       --warc-tempdir=DIRECTORY    location for temporary files created by the\n\
                                     WARC writer\n"),
    "\n",

    N_("\
Recursive download:\n"),
    N_("\
  -r,  --recursive                 specify recursive download\n"),
    N_("\
  -l,  --level=NUMBER              maximum recursion depth (inf or 0 for infinite)\n"),
    N_("\
       --delete-after              delete files locally after downloading them\n"),
    N_("\
  -k,  --convert-links             make links in downloaded HTML or CSS point to\n\
                                     local files\n"),
    N_("\
       --convert-file-only         convert the file part of the URLs only (usually known as the basename)\n"),
    N_("\
       --backups=N                 before writing file X, rotate up to N backup files\n"),

#ifdef __VMS
    N_("\
  -K,  --backup-converted          before converting file X, back up as X_orig\n"),
#else /* def __VMS */
    N_("\
  -K,  --backup-converted          before converting file X, back up as X.orig\n"),
#endif /* def __VMS [else] */
    N_("\
  -m,  --mirror                    shortcut for -N -r -l inf --no-remove-listing\n"),
    N_("\
  -p,  --page-requisites           get all images, etc. needed to display HTML page\n"),
    N_("\
       --strict-comments           turn on strict (SGML) handling of HTML comments\n"),
    "\n",

    N_("\
Recursive accept/reject:\n"),
    N_("\
  -A,  --accept=LIST               comma-separated list of accepted extensions\n"),
    N_("\
  -R,  --reject=LIST               comma-separated list of rejected extensions\n"),
    N_("\
       --accept-regex=REGEX        regex matching accepted URLs\n"),
    N_("\
       --reject-regex=REGEX        regex matching rejected URLs\n"),
#if defined HAVE_LIBPCRE || defined HAVE_LIBPCRE2
    N_("\
       --regex-type=TYPE           regex type (posix|pcre)\n"),
#else
    N_("\
       --regex-type=TYPE           regex type (posix)\n"),
#endif
    N_("\
  -D,  --domains=LIST              comma-separated list of accepted domains\n"),
    N_("\
       --exclude-domains=LIST      comma-separated list of rejected domains\n"),
    N_("\
       --follow-ftp                follow FTP links from HTML documents\n"),
    N_("\
       --follow-tags=LIST          comma-separated list of followed HTML tags\n"),
    N_("\
       --ignore-tags=LIST          comma-separated list of ignored HTML tags\n"),
    N_("\
  -H,  --span-hosts                go to foreign hosts when recursive\n"),
    N_("\
  -L,  --relative                  follow relative links only\n"),
    N_("\
  -I,  --include-directories=LIST  list of allowed directories\n"),
    N_("\
       --trust-server-names        use the name specified by the redirection\n\
                                     URL's last component\n"),
    N_("\
  -X,  --exclude-directories=LIST  list of excluded directories\n"),
    N_("\
  -np, --no-parent                 don't ascend to the parent directory\n"),
    "\n",
    N_("Email bug reports, questions, discussions to <bug-wget@gnu.org>\n"),
    N_("and/or open issues at https://savannah.gnu.org/bugs/?func=additem&group=wget.\n")
  };

  size_t i;

  if (printf (_("GNU Wget %s, a non-interactive network retriever.\n"),
              version_string) < 0)
    exit (WGET_EXIT_IO_FAIL);
  if (print_usage (0) < 0)
    exit (WGET_EXIT_IO_FAIL);

  for (i = 0; i < countof (help); i++)
    if (fputs (_(help[i]), stdout) < 0)
      exit (WGET_EXIT_IO_FAIL);
#endif /* TESTING */
  exit (WGET_EXIT_SUCCESS);
}