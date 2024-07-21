print_help (void)
{
  /* We split the help text this way to ease translation of individual
     entries.  */
  static const char *help[] = {
    "\n",
    N_("\
Mandatory arguments to long options are mandatory for short options too.\n\n"),
    N_("\
Startup:\n"),
    N_("\
  -V,  --version           display the version of Wget and exit.\n"),
    N_("\
  -h,  --help              print this help.\n"),
    N_("\
  -b,  --background        go to background after startup.\n"),
    N_("\
  -e,  --execute=COMMAND   execute a `.wgetrc'-style command.\n"),
    "\n",

    N_("\
Logging and input file:\n"),
    N_("\
  -o,  --output-file=FILE    log messages to FILE.\n"),
    N_("\
  -a,  --append-output=FILE  append messages to FILE.\n"),
#ifdef ENABLE_DEBUG
    N_("\
  -d,  --debug               print lots of debugging information.\n"),
#endif
#ifdef USE_WATT32
    N_("\
       --wdebug              print Watt-32 debug output.\n"),
#endif
    N_("\
  -q,  --quiet               quiet (no output).\n"),
    N_("\
  -v,  --verbose             be verbose (this is the default).\n"),
    N_("\
  -nv, --no-verbose          turn off verboseness, without being quiet.\n"),
    N_("\
  -i,  --input-file=FILE     download URLs found in local or external FILE.\n"),
    N_("\
  -F,  --force-html          treat input file as HTML.\n"),
    N_("\
  -B,  --base=URL            resolves HTML input-file links (-i -F)\n\
                             relative to URL.\n"),
    "\n",

    N_("\
Download:\n"),
    N_("\
  -t,  --tries=NUMBER            set number of retries to NUMBER (0 unlimits).\n"),
    N_("\
       --retry-connrefused       retry even if connection is refused.\n"),
    N_("\
  -O,  --output-document=FILE    write documents to FILE.\n"),
    N_("\
  -nc, --no-clobber              skip downloads that would download to\n\
                                 existing files.\n"),
    N_("\
  -c,  --continue                resume getting a partially-downloaded file.\n"),
    N_("\
       --progress=TYPE           select progress gauge type.\n"),
    N_("\
  -N,  --timestamping            don't re-retrieve files unless newer than\n\
                                 local.\n"),
    N_("\
  --no-use-server-timestamps     don't set the local file's timestamp by\n\
                                 the one on the server.\n"),
    N_("\
  -S,  --server-response         print server response.\n"),
    N_("\
       --spider                  don't download anything.\n"),
    N_("\
  -T,  --timeout=SECONDS         set all timeout values to SECONDS.\n"),
    N_("\
       --dns-timeout=SECS        set the DNS lookup timeout to SECS.\n"),
    N_("\
       --connect-timeout=SECS    set the connect timeout to SECS.\n"),
    N_("\
       --read-timeout=SECS       set the read timeout to SECS.\n"),
    N_("\
  -w,  --wait=SECONDS            wait SECONDS between retrievals.\n"),
    N_("\
       --waitretry=SECONDS       wait 1..SECONDS between retries of a retrieval.\n"),
    N_("\
       --random-wait             wait from 0.5*WAIT...1.5*WAIT secs between retrievals.\n"),
    N_("\
       --no-proxy                explicitly turn off proxy.\n"),
    N_("\
  -Q,  --quota=NUMBER            set retrieval quota to NUMBER.\n"),
    N_("\
       --bind-address=ADDRESS    bind to ADDRESS (hostname or IP) on local host.\n"),
    N_("\
       --limit-rate=RATE         limit download rate to RATE.\n"),
    N_("\
       --no-dns-cache            disable caching DNS lookups.\n"),
    N_("\
       --restrict-file-names=OS  restrict chars in file names to ones OS allows.\n"),
    N_("\
       --ignore-case             ignore case when matching files/directories.\n"),
#ifdef ENABLE_IPV6
    N_("\
  -4,  --inet4-only              connect only to IPv4 addresses.\n"),
    N_("\
  -6,  --inet6-only              connect only to IPv6 addresses.\n"),
    N_("\
       --prefer-family=FAMILY    connect first to addresses of specified family,\n\
                                 one of IPv6, IPv4, or none.\n"),
#endif
    N_("\
       --user=USER               set both ftp and http user to USER.\n"),
    N_("\
       --password=PASS           set both ftp and http password to PASS.\n"),
    N_("\
       --ask-password            prompt for passwords.\n"),
    N_("\
       --no-iri                  turn off IRI support.\n"),
    N_("\
       --local-encoding=ENC      use ENC as the local encoding for IRIs.\n"),
    N_("\
       --remote-encoding=ENC     use ENC as the default remote encoding.\n"),
    "\n",

    N_("\
Directories:\n"),
    N_("\
  -nd, --no-directories           don't create directories.\n"),
    N_("\
  -x,  --force-directories        force creation of directories.\n"),
    N_("\
  -nH, --no-host-directories      don't create host directories.\n"),
    N_("\
       --protocol-directories     use protocol name in directories.\n"),
    N_("\
  -P,  --directory-prefix=PREFIX  save files to PREFIX/...\n"),
    N_("\
       --cut-dirs=NUMBER          ignore NUMBER remote directory components.\n"),
    "\n",

    N_("\
HTTP options:\n"),
    N_("\
       --http-user=USER        set http user to USER.\n"),
    N_("\
       --http-password=PASS    set http password to PASS.\n"),
    N_("\
       --no-cache              disallow server-cached data.\n"),
    N_ ("\
       --default-page=NAME     Change the default page name (normally\n\
                               this is `index.html'.).\n"),
    N_("\
  -E,  --adjust-extension      save HTML/CSS documents with proper extensions.\n"),
    N_("\
       --ignore-length         ignore `Content-Length' header field.\n"),
    N_("\
       --header=STRING         insert STRING among the headers.\n"),
    N_("\
       --max-redirect          maximum redirections allowed per page.\n"),
    N_("\
       --proxy-user=USER       set USER as proxy username.\n"),
    N_("\
       --proxy-password=PASS   set PASS as proxy password.\n"),
    N_("\
       --referer=URL           include `Referer: URL' header in HTTP request.\n"),
    N_("\
       --save-headers          save the HTTP headers to file.\n"),
    N_("\
  -U,  --user-agent=AGENT      identify as AGENT instead of Wget/VERSION.\n"),
    N_("\
       --no-http-keep-alive    disable HTTP keep-alive (persistent connections).\n"),
    N_("\
       --no-cookies            don't use cookies.\n"),
    N_("\
       --load-cookies=FILE     load cookies from FILE before session.\n"),
    N_("\
       --save-cookies=FILE     save cookies to FILE after session.\n"),
    N_("\
       --keep-session-cookies  load and save session (non-permanent) cookies.\n"),
    N_("\
       --post-data=STRING      use the POST method; send STRING as the data.\n"),
    N_("\
       --post-file=FILE        use the POST method; send contents of FILE.\n"),
    N_("\
       --content-disposition   honor the Content-Disposition header when\n\
                               choosing local file names (EXPERIMENTAL).\n"),
    N_("\
       --auth-no-challenge     send Basic HTTP authentication information\n\
                               without first waiting for the server's\n\
                               challenge.\n"),
    "\n",

#ifdef HAVE_SSL
    N_("\
HTTPS (SSL/TLS) options:\n"),
    N_("\
       --secure-protocol=PR     choose secure protocol, one of auto, SSLv2,\n\
                                SSLv3, and TLSv1.\n"),
    N_("\
       --no-check-certificate   don't validate the server's certificate.\n"),
    N_("\
       --certificate=FILE       client certificate file.\n"),
    N_("\
       --certificate-type=TYPE  client certificate type, PEM or DER.\n"),
    N_("\
       --private-key=FILE       private key file.\n"),
    N_("\
       --private-key-type=TYPE  private key type, PEM or DER.\n"),
    N_("\
       --ca-certificate=FILE    file with the bundle of CA's.\n"),
    N_("\
       --ca-directory=DIR       directory where hash list of CA's is stored.\n"),
    N_("\
       --random-file=FILE       file with random data for seeding the SSL PRNG.\n"),
    N_("\
       --egd-file=FILE          file naming the EGD socket with random data.\n"),
    "\n",
#endif /* HAVE_SSL */

    N_("\
FTP options:\n"),
#ifdef __VMS
    N_("\
       --ftp-stmlf             Use Stream_LF format for all binary FTP files.\n"),
#endif /* def __VMS */
    N_("\
       --ftp-user=USER         set ftp user to USER.\n"),
    N_("\
       --ftp-password=PASS     set ftp password to PASS.\n"),
    N_("\
       --no-remove-listing     don't remove `.listing' files.\n"),
    N_("\
       --no-glob               turn off FTP file name globbing.\n"),
    N_("\
       --no-passive-ftp        disable the \"passive\" transfer mode.\n"),
    N_("\
       --retr-symlinks         when recursing, get linked-to files (not dir).\n"),
    "\n",

    N_("\
Recursive download:\n"),
    N_("\
  -r,  --recursive          specify recursive download.\n"),
    N_("\
  -l,  --level=NUMBER       maximum recursion depth (inf or 0 for infinite).\n"),
    N_("\
       --delete-after       delete files locally after downloading them.\n"),
    N_("\
  -k,  --convert-links      make links in downloaded HTML or CSS point to\n\
                            local files.\n"),
#ifdef __VMS
    N_("\
  -K,  --backup-converted   before converting file X, back up as X_orig.\n"),
#else /* def __VMS */
    N_("\
  -K,  --backup-converted   before converting file X, back up as X.orig.\n"),
#endif /* def __VMS [else] */
    N_("\
  -m,  --mirror             shortcut for -N -r -l inf --no-remove-listing.\n"),
    N_("\
  -p,  --page-requisites    get all images, etc. needed to display HTML page.\n"),
    N_("\
       --strict-comments    turn on strict (SGML) handling of HTML comments.\n"),
    "\n",

    N_("\
Recursive accept/reject:\n"),
    N_("\
  -A,  --accept=LIST               comma-separated list of accepted extensions.\n"),
    N_("\
  -R,  --reject=LIST               comma-separated list of rejected extensions.\n"),
    N_("\
  -D,  --domains=LIST              comma-separated list of accepted domains.\n"),
    N_("\
       --exclude-domains=LIST      comma-separated list of rejected domains.\n"),
    N_("\
       --follow-ftp                follow FTP links from HTML documents.\n"),
    N_("\
       --follow-tags=LIST          comma-separated list of followed HTML tags.\n"),
    N_("\
       --ignore-tags=LIST          comma-separated list of ignored HTML tags.\n"),
    N_("\
  -H,  --span-hosts                go to foreign hosts when recursive.\n"),
    N_("\
  -L,  --relative                  follow relative links only.\n"),
    N_("\
  -I,  --include-directories=LIST  list of allowed directories.\n"),
    N_("\
  -X,  --exclude-directories=LIST  list of excluded directories.\n"),
    N_("\
  -np, --no-parent                 don't ascend to the parent directory.\n"),
    "\n",

    N_("Mail bug reports and suggestions to <bug-wget@gnu.org>.\n")
  };

  size_t i;

  printf (_("GNU Wget %s, a non-interactive network retriever.\n"),
          version_string);
  print_usage (0);

  for (i = 0; i < countof (help); i++)
    fputs (_(help[i]), stdout);

  exit (0);
}