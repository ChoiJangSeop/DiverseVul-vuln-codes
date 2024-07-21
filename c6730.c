std::vector<Option> get_rgw_options() {
  return std::vector<Option>({
    Option("rgw_acl_grants_max_num", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(100)
    .set_description("Max number of ACL grants in a single request"),

    Option("rgw_rados_tracing", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("true if LTTng-UST tracepoints should be enabled"),

    Option("rgw_op_tracing", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("true if LTTng-UST tracepoints should be enabled"),

    Option("rgw_max_chunk_size", Option::TYPE_SIZE, Option::LEVEL_ADVANCED)
    .set_default(4_M)
    .set_description("Set RGW max chunk size")
    .set_long_description(
        "The chunk size is the size of RADOS I/O requests that RGW sends when accessing "
        "data objects. RGW read and write operation will never request more than this amount "
        "in a single request. This also defines the rgw object head size, as head operations "
        "need to be atomic, and anything larger than this would require more than a single "
        "operation."),

    Option("rgw_put_obj_min_window_size", Option::TYPE_SIZE, Option::LEVEL_ADVANCED)
    .set_default(16_M)
    .set_description("The minimum RADOS write window size (in bytes).")
    .set_long_description(
        "The window size determines the total concurrent RADOS writes of a single rgw object. "
        "When writing an object RGW will send multiple chunks to RADOS. The total size of the "
        "writes does not exceed the window size. The window size can be automatically "
        "in order to better utilize the pipe.")
    .add_see_also({"rgw_put_obj_max_window_size", "rgw_max_chunk_size"}),

    Option("rgw_put_obj_max_window_size", Option::TYPE_SIZE, Option::LEVEL_ADVANCED)
    .set_default(64_M)
    .set_description("The maximum RADOS write window size (in bytes).")
    .set_long_description("The window size may be dynamically adjusted, but will not surpass this value.")
    .add_see_also({"rgw_put_obj_min_window_size", "rgw_max_chunk_size"}),

    Option("rgw_max_put_size", Option::TYPE_SIZE, Option::LEVEL_ADVANCED)
    .set_default(5_G)
    .set_description("Max size (in bytes) of regular (non multi-part) object upload.")
    .set_long_description(
        "Plain object upload is capped at this amount of data. In order to upload larger "
        "objects, a special upload mechanism is required. The S3 API provides the "
        "multi-part upload, and Swift provides DLO and SLO."),

    Option("rgw_max_put_param_size", Option::TYPE_SIZE, Option::LEVEL_ADVANCED)
    .set_default(1_M)
    .set_description("The maximum size (in bytes) of data input of certain RESTful requests."),

    Option("rgw_max_attr_size", Option::TYPE_SIZE, Option::LEVEL_ADVANCED)
    .set_default(0)
    .set_description("The maximum length of metadata value. 0 skips the check"),

    Option("rgw_max_attr_name_len", Option::TYPE_UINT, Option::LEVEL_ADVANCED)
    .set_default(0)
    .set_description("The maximum length of metadata name. 0 skips the check"),

    Option("rgw_max_attrs_num_in_req", Option::TYPE_UINT, Option::LEVEL_ADVANCED)
    .set_default(0)
    .set_description("The maximum number of metadata items that can be put via single request"),

    Option("rgw_override_bucket_index_max_shards", Option::TYPE_UINT, Option::LEVEL_DEV)
    .set_default(0)
    .set_description(""),

    Option("rgw_bucket_index_max_aio", Option::TYPE_UINT, Option::LEVEL_ADVANCED)
    .set_default(8)
    .set_description("Max number of concurrent RADOS requests when handling bucket shards."),

    Option("rgw_enable_quota_threads", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(true)
    .set_description("Enables the quota maintenance thread.")
    .set_long_description(
        "The quota maintenance thread is responsible for quota related maintenance work. "
        "The thread itself can be disabled, but in order for quota to work correctly, at "
        "least one RGW in each zone needs to have this thread running. Having the thread "
        "enabled on multiple RGW processes within the same zone can spread "
        "some of the maintenance work between them.")
    .add_see_also({"rgw_enable_gc_threads", "rgw_enable_lc_threads"}),

    Option("rgw_enable_gc_threads", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(true)
    .set_description("Enables the garbage collection maintenance thread.")
    .set_long_description(
        "The garbage collection maintenance thread is responsible for garbage collector "
        "maintenance work. The thread itself can be disabled, but in order for garbage "
        "collection to work correctly, at least one RGW in each zone needs to have this "
        "thread running.  Having the thread enabled on multiple RGW processes within the "
        "same zone can spread some of the maintenance work between them.")
    .add_see_also({"rgw_enable_quota_threads", "rgw_enable_lc_threads"}),

    Option("rgw_enable_lc_threads", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(true)
    .set_description("Enables the lifecycle maintenance thread. This is required on at least on rgw for each zone.")
    .set_long_description(
        "The lifecycle maintenance thread is responsible for lifecycle related maintenance "
        "work. The thread itself can be disabled, but in order for lifecycle to work "
        "correctly, at least one RGW in each zone needs to have this thread running. Having"
        "the thread enabled on multiple RGW processes within the same zone can spread "
        "some of the maintenance work between them.")
    .add_see_also({"rgw_enable_gc_threads", "rgw_enable_quota_threads"}),

    Option("rgw_data", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("/var/lib/ceph/radosgw/$cluster-$id")
    .set_flag(Option::FLAG_NO_MON_UPDATE)
    .set_description("Alternative location for RGW configuration.")
    .set_long_description(
        "If this is set, the different Ceph system configurables (such as the keyring file "
        "will be located in the path that is specified here. "),

    Option("rgw_enable_apis", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("s3, s3website, swift, swift_auth, admin")
    .set_description("A list of set of RESTful APIs that rgw handles."),

    Option("rgw_cache_enabled", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(true)
    .set_description("Enable RGW metadata cache.")
    .set_long_description(
        "The metadata cache holds metadata entries that RGW requires for processing "
        "requests. Metadata entries can be user info, bucket info, and bucket instance "
        "info. If not found in the cache, entries will be fetched from the backing "
        "RADOS store.")
    .add_see_also("rgw_cache_lru_size"),

    Option("rgw_cache_lru_size", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(10000)
    .set_description("Max number of items in RGW metadata cache.")
    .set_long_description(
        "When full, the RGW metadata cache evicts least recently used entries.")
    .add_see_also("rgw_cache_enabled"),

    Option("rgw_socket_path", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("RGW FastCGI socket path (for FastCGI over Unix domain sockets).")
    .add_see_also("rgw_fcgi_socket_backlog"),

    Option("rgw_host", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("RGW FastCGI host name (for FastCGI over TCP)")
    .add_see_also({"rgw_port", "rgw_fcgi_socket_backlog"}),

    Option("rgw_port", Option::TYPE_STR, Option::LEVEL_BASIC)
    .set_default("")
    .set_description("RGW FastCGI port number (for FastCGI over TCP)")
    .add_see_also({"rgw_host", "rgw_fcgi_socket_backlog"}),

    Option("rgw_dns_name", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("The host name that RGW uses.")
    .set_long_description(
        "This is Needed for virtual hosting of buckets to work properly, unless configured "
        "via zonegroup configuration."),

    Option("rgw_dns_s3website_name", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("The host name that RGW uses for static websites (S3)")
    .set_long_description(
        "This is needed for virtual hosting of buckets, unless configured via zonegroup "
        "configuration."),

    Option("rgw_content_length_compat", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("Multiple content length headers compatibility")
    .set_long_description(
        "Try to handle requests with abiguous multiple content length headers "
        "(Content-Length, Http-Content-Length)."),

    Option("rgw_lifecycle_work_time", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("00:00-06:00")
    .set_description("Lifecycle allowed work time")
    .set_long_description("Local time window in which the lifecycle maintenance thread can work."),

    Option("rgw_lc_lock_max_time", Option::TYPE_INT, Option::LEVEL_DEV)
    .set_default(60)
    .set_description(""),

    Option("rgw_lc_max_objs", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(32)
    .set_description("Number of lifecycle data shards")
    .set_long_description(
          "Number of RADOS objects to use for storing lifecycle index. This can affect "
          "concurrency of lifecycle maintenance, but requires multiple RGW processes "
          "running on the zone to be utilized."),

    Option("rgw_lc_max_rules", Option::TYPE_UINT, Option::LEVEL_ADVANCED)
    .set_default(1000)
    .set_description("Max number of lifecycle rules set on one bucket")
    .set_long_description("Number of lifecycle rules set on one bucket should be limited."),

    Option("rgw_lc_debug_interval", Option::TYPE_INT, Option::LEVEL_DEV)
    .set_default(-1)
    .set_description(""),

    Option("rgw_mp_lock_max_time", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(600)
    .set_description("Multipart upload max completion time")
    .set_long_description(
        "Time length to allow completion of a multipart upload operation. This is done "
        "to prevent concurrent completions on the same object with the same upload id."),

    Option("rgw_script_uri", Option::TYPE_STR, Option::LEVEL_DEV)
    .set_default("")
    .set_description(""),

    Option("rgw_request_uri", Option::TYPE_STR, Option::LEVEL_DEV)
    .set_default("")
    .set_description(""),

    Option("rgw_ignore_get_invalid_range", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("Treat invalid (e.g., negative) range request as full")
    .set_long_description("Treat invalid (e.g., negative) range request "
			  "as request for the full object (AWS compatibility)"),

    Option("rgw_swift_url", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Swift-auth storage URL")
    .set_long_description(
        "Used in conjunction with rgw internal swift authentication. This affects the "
        "X-Storage-Url response header value.")
    .add_see_also("rgw_swift_auth_entry"),

    Option("rgw_swift_url_prefix", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("swift")
    .set_description("Swift URL prefix")
    .set_long_description("The URL path prefix for swift requests."),

    Option("rgw_swift_auth_url", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Swift auth URL")
    .set_long_description(
        "Default url to which RGW connects and verifies tokens for v1 auth (if not using "
        "internal swift auth)."),

    Option("rgw_swift_auth_entry", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("auth")
    .set_description("Swift auth URL prefix")
    .set_long_description("URL path prefix for internal swift auth requests.")
    .add_see_also("rgw_swift_url"),

    Option("rgw_swift_tenant_name", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Swift tenant name")
    .set_long_description("Tenant name that is used when constructing the swift path.")
    .add_see_also("rgw_swift_account_in_url"),

    Option("rgw_swift_account_in_url", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("Swift account encoded in URL")
    .set_long_description("Whether the swift account is encoded in the uri path (AUTH_<account>).")
    .add_see_also("rgw_swift_tenant_name"),

    Option("rgw_swift_enforce_content_length", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("Send content length when listing containers (Swift)")
    .set_long_description(
        "Whether content length header is needed when listing containers. When this is "
        "set to false, RGW will send extra info for each entry in the response."),

    Option("rgw_keystone_url", Option::TYPE_STR, Option::LEVEL_BASIC)
    .set_default("")
    .set_description("The URL to the Keystone server."),

    Option("rgw_keystone_admin_token", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("The admin token (shared secret) that is used for the Keystone requests."),

    Option("rgw_keystone_admin_user", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Keystone admin user."),

    Option("rgw_keystone_admin_password", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Keystone admin password."),

    Option("rgw_keystone_admin_tenant", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Keystone admin user tenant."),

    Option("rgw_keystone_admin_project", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Keystone admin user project (for Keystone v3)."),

    Option("rgw_keystone_admin_domain", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Keystone admin user domain (for Keystone v3)."),

    Option("rgw_keystone_barbican_user", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Keystone user to access barbican secrets."),

    Option("rgw_keystone_barbican_password", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Keystone password for barbican user."),

    Option("rgw_keystone_barbican_tenant", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Keystone barbican user tenant (Keystone v2.0)."),

    Option("rgw_keystone_barbican_project", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Keystone barbican user project (Keystone v3)."),

    Option("rgw_keystone_barbican_domain", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Keystone barbican user domain."),

    Option("rgw_keystone_api_version", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(2)
    .set_description("Version of Keystone API to use (2 or 3)."),

    Option("rgw_keystone_accepted_roles", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("Member, admin")
    .set_description("Only users with one of these roles will be served when doing Keystone authentication."),

    Option("rgw_keystone_accepted_admin_roles", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("List of roles allowing user to gain admin privileges (Keystone)."),

    Option("rgw_keystone_token_cache_size", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(10000)
    .set_description("Keystone token cache size")
    .set_long_description(
        "Max number of Keystone tokens that will be cached. Token that is not cached "
        "requires RGW to access the Keystone server when authenticating."),

    Option("rgw_keystone_revocation_interval", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(15_min)
    .set_description("Keystone cache revocation interval")
    .set_long_description(
        "Time (in seconds) that RGW waits between requests to Keystone for getting a list "
        "of revoked tokens. A revoked token might still be considered valid by RGW for "
        "this amount of time."),

    Option("rgw_keystone_verify_ssl", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(true)
    .set_description("Should RGW verify the Keystone server SSL certificate."),

    Option("rgw_keystone_implicit_tenants", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("RGW Keystone implicit tenants creation")
    .set_long_description(
        "Implicitly create new users in their own tenant with the same name when "
        "authenticating via Keystone."),

    Option("rgw_cross_domain_policy", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("<allow-access-from domain=\"*\" secure=\"false\" />")
    .set_description("RGW handle cross domain policy")
    .set_long_description("Returned cross domain policy when accessing the crossdomain.xml "
                          "resource (Swift compatiility)."),

    Option("rgw_healthcheck_disabling_path", Option::TYPE_STR, Option::LEVEL_DEV)
    .set_default("")
    .set_description("Swift health check api can be disabled if a file can be accessed in this path."),

    Option("rgw_s3_auth_use_rados", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(true)
    .set_description("Should S3 authentication use credentials stored in RADOS backend."),

    Option("rgw_s3_auth_use_keystone", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("Should S3 authentication use Keystone."),

    Option("rgw_s3_auth_order", Option::TYPE_STR, Option::LEVEL_ADVANCED)
     .set_default("external, local")
     .set_description("Authentication strategy order to use for s3 authentication")
     .set_long_description(
	  "Order of authentication strategies to try for s3 authentication, the allowed "
	   "options are a comma separated list of engines external, local. The "
	   "default order is to try all the externally configured engines before "
	   "attempting local rados based authentication"),

    Option("rgw_barbican_url", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("URL to barbican server."),

    Option("rgw_ldap_uri", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("ldaps://<ldap.your.domain>")
    .set_description("Space-separated list of LDAP servers in URI format."),

    Option("rgw_ldap_binddn", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("uid=admin,cn=users,dc=example,dc=com")
    .set_description("LDAP entry RGW will bind with (user match)."),

    Option("rgw_ldap_searchdn", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("cn=users,cn=accounts,dc=example,dc=com")
    .set_description("LDAP search base (basedn)."),

    Option("rgw_ldap_dnattr", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("uid")
    .set_description("LDAP attribute containing RGW user names (to form binddns)."),

    Option("rgw_ldap_secret", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("/etc/openldap/secret")
    .set_description("Path to file containing credentials for rgw_ldap_binddn."),

    Option("rgw_s3_auth_use_ldap", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("Should S3 authentication use LDAP."),

    Option("rgw_ldap_searchfilter", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("LDAP search filter."),

    Option("rgw_admin_entry", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("admin")
    .set_description("Path prefix to be used for accessing RGW RESTful admin API."),

    Option("rgw_enforce_swift_acls", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(true)
    .set_description("RGW enforce swift acls")
    .set_long_description(
        "Should RGW enforce special Swift-only ACLs. Swift has a special ACL that gives "
        "permission to access all objects in a container."),

    Option("rgw_swift_token_expiration", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(1_day)
    .set_description("Expiration time (in seconds) for token generated through RGW Swift auth."),

    Option("rgw_print_continue", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(true)
    .set_description("RGW support of 100-continue")
    .set_long_description(
        "Should RGW explicitly send 100 (continue) responses. This is mainly relevant when "
        "using FastCGI, as some FastCGI modules do not fully support this feature."),

    Option("rgw_print_prohibited_content_length", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("RGW RFC-7230 compatibility")
    .set_long_description(
        "Specifies whether RGW violates RFC 7230 and sends Content-Length with 204 or 304 "
        "statuses."),

    Option("rgw_remote_addr_param", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("REMOTE_ADDR")
    .set_description("HTTP header that holds the remote address in incoming requests.")
    .set_long_description(
        "RGW will use this header to extract requests origin. When RGW runs behind "
        "a reverse proxy, the remote address header will point at the proxy's address "
        "and not at the originator's address. Therefore it is sometimes possible to "
        "have the proxy add the originator's address in a separate HTTP header, which "
        "will allow RGW to log it correctly."
        )
    .add_see_also("rgw_enable_ops_log"),

    Option("rgw_op_thread_timeout", Option::TYPE_INT, Option::LEVEL_DEV)
    .set_default(10*60)
    .set_description("Timeout for async rados coroutine operations."),

    Option("rgw_op_thread_suicide_timeout", Option::TYPE_INT, Option::LEVEL_DEV)
    .set_default(0)
    .set_description(""),

    Option("rgw_thread_pool_size", Option::TYPE_INT, Option::LEVEL_BASIC)
    .set_default(512)
    .set_description("RGW requests handling thread pool size.")
    .set_long_description(
        "This parameter determines the number of concurrent requests RGW can process "
        "when using either the civetweb, or the fastcgi frontends. The higher this "
        "number is, RGW will be able to deal with more concurrent requests at the "
        "cost of more resource utilization."),

    Option("rgw_num_control_oids", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(8)
    .set_description("Number of control objects used for cross-RGW communication.")
    .set_long_description(
        "RGW uses certain control objects to send messages between different RGW "
        "processes running on the same zone. These messages include metadata cache "
        "invalidation info that is being sent when metadata is modified (such as "
        "user or bucket information). A higher number of control objects allows "
        "better concurrency of these messages, at the cost of more resource "
        "utilization."),

    Option("rgw_num_rados_handles", Option::TYPE_UINT, Option::LEVEL_ADVANCED)
    .set_default(1)
    .set_description("Number of librados handles that RGW uses.")
    .set_long_description(
        "This param affects the number of separate librados handles it uses to "
        "connect to the RADOS backend, which directly affects the number of connections "
        "RGW will have to each OSD. A higher number affects resource utilization."),

    Option("rgw_verify_ssl", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(true)
    .set_description("Should RGW verify SSL when connecing to a remote HTTP server")
    .set_long_description(
        "RGW can send requests to other RGW servers (e.g., in multi-site sync work). "
        "This configurable selects whether RGW should verify the certificate for "
        "the remote peer and host.")
    .add_see_also("rgw_keystone_verify_ssl"),

    Option("rgw_nfs_lru_lanes", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(5)
    .set_description(""),

    Option("rgw_nfs_lru_lane_hiwat", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(911)
    .set_description(""),

    Option("rgw_nfs_fhcache_partitions", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(3)
    .set_description(""),

    Option("rgw_nfs_fhcache_size", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(2017)
    .set_description(""),

    Option("rgw_nfs_namespace_expire_secs", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(300)
    .set_min(1)
    .set_description(""),

    Option("rgw_nfs_max_gc", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(300)
    .set_min(1)
    .set_description(""),

    Option("rgw_nfs_write_completion_interval_s", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(10)
    .set_description(""),

    Option("rgw_zone", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Zone name")
    .add_see_also({"rgw_zonegroup", "rgw_realm"}),

    Option("rgw_zone_root_pool", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default(".rgw.root")
    .set_description("Zone root pool name")
    .set_long_description(
        "The zone root pool, is the pool where the RGW zone configuration located."
    )
    .add_see_also({"rgw_zonegroup_root_pool", "rgw_realm_root_pool", "rgw_period_root_pool"}),

    Option("rgw_default_zone_info_oid", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("default.zone")
    .set_description("Default zone info object id")
    .set_long_description(
        "Name of the RADOS object that holds the default zone information."
    ),

    Option("rgw_region", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Region name")
    .set_long_description(
        "Obsolete config option. The rgw_zonegroup option should be used instead.")
    .add_see_also("rgw_zonegroup"),

    Option("rgw_region_root_pool", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default(".rgw.root")
    .set_description("Region root pool")
    .set_long_description(
        "Obsolete config option. The rgw_zonegroup_root_pool should be used instead.")
    .add_see_also("rgw_zonegroup_root_pool"),

    Option("rgw_default_region_info_oid", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("default.region")
    .set_description("Default region info object id")
    .set_long_description(
        "Obsolete config option. The rgw_default_zonegroup_info_oid should be used instead.")
    .add_see_also("rgw_default_zonegroup_info_oid"),

    Option("rgw_zonegroup", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Zonegroup name")
    .add_see_also({"rgw_zone", "rgw_realm"}),

    Option("rgw_zonegroup_root_pool", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default(".rgw.root")
    .set_description("Zonegroup root pool")
    .set_long_description(
        "The zonegroup root pool, is the pool where the RGW zonegroup configuration located."
    )
    .add_see_also({"rgw_zone_root_pool", "rgw_realm_root_pool", "rgw_period_root_pool"}),

    Option("rgw_default_zonegroup_info_oid", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("default.zonegroup")
    .set_description(""),

    Option("rgw_realm", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description(""),

    Option("rgw_realm_root_pool", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default(".rgw.root")
    .set_description("Realm root pool")
    .set_long_description(
        "The realm root pool, is the pool where the RGW realm configuration located."
    )
    .add_see_also({"rgw_zonegroup_root_pool", "rgw_zone_root_pool", "rgw_period_root_pool"}),

    Option("rgw_default_realm_info_oid", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("default.realm")
    .set_description(""),

    Option("rgw_period_root_pool", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default(".rgw.root")
    .set_description("Period root pool")
    .set_long_description(
        "The realm root pool, is the pool where the RGW realm configuration located."
    )
    .add_see_also({"rgw_zonegroup_root_pool", "rgw_zone_root_pool", "rgw_realm_root_pool"}),

    Option("rgw_period_latest_epoch_info_oid", Option::TYPE_STR, Option::LEVEL_DEV)
    .set_default(".latest_epoch")
    .set_description(""),

    Option("rgw_log_nonexistent_bucket", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("Should RGW log operations on bucket that does not exist")
    .set_long_description(
        "This config option applies to the ops log. When this option is set, the ops log "
        "will log operations that are sent to non existing buckets. These operations "
        "inherently fail, and do not correspond to a specific user.")
    .add_see_also("rgw_enable_ops_log"),

    Option("rgw_log_object_name", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("%Y-%m-%d-%H-%i-%n")
    .set_description("Ops log object name format")
    .set_long_description(
        "Defines the format of the RADOS objects names that ops log uses to store ops "
        "log data")
    .add_see_also("rgw_enable_ops_log"),

    Option("rgw_log_object_name_utc", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("Should ops log object name based on UTC")
    .set_long_description(
        "If set, the names of the RADOS objects that hold the ops log data will be based "
        "on UTC time zone. If not set, it will use the local time zone.")
    .add_see_also({"rgw_enable_ops_log", "rgw_log_object_name"}),

    Option("rgw_usage_max_shards", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(32)
    .set_description("Number of shards for usage log.")
    .set_long_description(
        "The number of RADOS objects that RGW will use in order to store the usage log "
        "data.")
    .add_see_also("rgw_enable_usage_log"),

    Option("rgw_usage_max_user_shards", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(1)
    .set_min(1)
    .set_description("Number of shards for single user in usage log")
    .set_long_description(
        "The number of shards that a single user will span over in the usage log.")
    .add_see_also("rgw_enable_usage_log"),

    Option("rgw_enable_ops_log", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("Enable ops log")
    .add_see_also({"rgw_log_nonexistent_bucket", "rgw_log_object_name", "rgw_ops_log_rados",
               "rgw_ops_log_socket_path"}),

    Option("rgw_enable_usage_log", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("Enable usage log")
    .add_see_also("rgw_usage_max_shards"),

    Option("rgw_ops_log_rados", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(true)
    .set_description("Use RADOS for ops log")
    .set_long_description(
       "If set, RGW will store ops log information in RADOS.")
    .add_see_also({"rgw_enable_ops_log"}),

    Option("rgw_ops_log_socket_path", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Unix domain socket path for ops log.")
    .set_long_description(
        "Path to unix domain socket that RGW will listen for connection on. When connected, "
        "RGW will send ops log data through it.")
    .add_see_also({"rgw_enable_ops_log", "rgw_ops_log_data_backlog"}),

    Option("rgw_ops_log_data_backlog", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(5 << 20)
    .set_description("Ops log socket backlog")
    .set_long_description(
        "Maximum amount of data backlog that RGW can keep when ops log is configured to "
        "send info through unix domain socket. When data backlog is higher than this, "
        "ops log entries will be lost. In order to avoid ops log information loss, the "
        "listener needs to clear data (by reading it) quickly enough.")
    .add_see_also({"rgw_enable_ops_log", "rgw_ops_log_socket_path"}),

    Option("rgw_fcgi_socket_backlog", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(1024)
    .set_description("FastCGI socket connection backlog")
    .set_long_description(
        "Size of FastCGI connection backlog. This reflects the maximum number of new "
        "connection requests that RGW can handle concurrently without dropping any. ")
    .add_see_also({"rgw_host", "rgw_socket_path"}),

    Option("rgw_usage_log_flush_threshold", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(1024)
    .set_description("Number of entries in usage log before flushing")
    .set_long_description(
        "This is the max number of entries that will be held in the usage log, before it "
        "will be flushed to the backend. Note that the usage log is periodically flushed, "
        "even if number of entries does not reach this threshold. A usage log entry "
        "corresponds to one or more operations on a single bucket.i")
    .add_see_also({"rgw_enable_usage_log", "rgw_usage_log_tick_interval"}),

    Option("rgw_usage_log_tick_interval", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(30)
    .set_description("Number of seconds between usage log flush cycles")
    .set_long_description(
        "The number of seconds between consecutive usage log flushes. The usage log will "
        "also flush itself to the backend if the number of pending entries reaches a "
        "certain threshold.")
    .add_see_also({"rgw_enable_usage_log", "rgw_usage_log_flush_threshold"}),

    Option("rgw_init_timeout", Option::TYPE_INT, Option::LEVEL_BASIC)
    .set_default(300)
    .set_description("Initialization timeout")
    .set_long_description(
        "The time length (in seconds) that RGW will allow for its initialization. RGW "
        "process will give up and quit if initialization is not complete after this amount "
        "of time."),

    Option("rgw_mime_types_file", Option::TYPE_STR, Option::LEVEL_BASIC)
    .set_default("/etc/mime.types")
    .set_description("Path to local mime types file")
    .set_long_description(
        "The mime types file is needed in Swift when uploading an object. If object's "
        "content type is not specified, RGW will use data from this file to assign "
        "a content type to the object."),

    Option("rgw_gc_max_objs", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(32)
    .set_description("Number of shards for garbage collector data")
    .set_long_description(
        "The number of garbage collector data shards, is the number of RADOS objects that "
        "RGW will use to store the garbage collection information on.")
    .add_see_also({"rgw_gc_obj_min_wait", "rgw_gc_processor_max_time", "rgw_gc_processor_period", "rgw_gc_max_concurrent_io"}),

    Option("rgw_gc_obj_min_wait", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(2_hr)
    .set_description("Garabge collection object expiration time")
    .set_long_description(
       "The length of time (in seconds) that the RGW collector will wait before purging "
       "a deleted object's data. RGW will not remove object immediately, as object could "
       "still have readers. A mechanism exists to increase the object's expiration time "
       "when it's being read.")
    .add_see_also({"rgw_gc_max_objs", "rgw_gc_processor_max_time", "rgw_gc_processor_period", "rgw_gc_max_concurrent_io"}),

    Option("rgw_gc_processor_max_time", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(1_hr)
    .set_description("Length of time GC processor can lease shard")
    .set_long_description(
        "Garbage collection thread in RGW process holds a lease on its data shards. These "
        "objects contain the information about the objects that need to be removed. RGW "
        "takes a lease in order to prevent multiple RGW processes from handling the same "
        "objects concurrently. This time signifies that maximum amount of time that RGW "
        "is allowed to hold that lease. In the case where RGW goes down uncleanly, this "
        "is the amount of time where processing of that data shard will be blocked.")
    .add_see_also({"rgw_gc_max_objs", "rgw_gc_obj_min_wait", "rgw_gc_processor_period", "rgw_gc_max_concurrent_io"}),

    Option("rgw_gc_processor_period", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(1_hr)
    .set_description("Garbage collector cycle run time")
    .set_long_description(
        "The amount of time between the start of consecutive runs of the garbage collector "
        "threads. If garbage collector runs takes more than this period, it will not wait "
        "before running again.")
    .add_see_also({"rgw_gc_max_objs", "rgw_gc_obj_min_wait", "rgw_gc_processor_max_time", "rgw_gc_max_concurrent_io", "rgw_gc_max_trim_chunk"}),

    Option("rgw_gc_max_concurrent_io", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(10)
    .set_description("Max concurrent RADOS IO operations for garbage collection")
    .set_long_description(
        "The maximum number of concurrent IO operations that the RGW garbage collection "
        "thread will use when purging old data.")
    .add_see_also({"rgw_gc_max_objs", "rgw_gc_obj_min_wait", "rgw_gc_processor_max_time", "rgw_gc_max_trim_chunk"}),

    Option("rgw_gc_max_trim_chunk", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(16)
    .set_description("Max number of keys to remove from garbage collector log in a single operation")
    .add_see_also({"rgw_gc_max_objs", "rgw_gc_obj_min_wait", "rgw_gc_processor_max_time", "rgw_gc_max_concurrent_io"}),

    Option("rgw_s3_success_create_obj_status", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(0)
    .set_description("HTTP return code override for object creation")
    .set_long_description(
        "If not zero, this is the HTTP return code that will be returned on a successful S3 "
        "object creation."),

    Option("rgw_resolve_cname", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("Support vanity domain names via CNAME")
    .set_long_description(
        "If true, RGW will query DNS when detecting that it's serving a request that was "
        "sent to a host in another domain. If a CNAME record is configured for that domain "
        "it will use it instead. This gives user to have the ability of creating a unique "
        "domain of their own to point at data in their bucket."),

    Option("rgw_obj_stripe_size", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(4_M)
    .set_description("RGW object stripe size")
    .set_long_description(
        "The size of an object stripe for RGW objects. This is the maximum size a backing "
        "RADOS object will have. RGW objects that are larger than this will span over "
        "multiple objects."),

    Option("rgw_extended_http_attrs", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("RGW support extended HTTP attrs")
    .set_long_description(
        "Add new set of attributes that could be set on an object. These extra attributes "
        "can be set through HTTP header fields when putting the objects. If set, these "
        "attributes will return as HTTP fields when doing GET/HEAD on the object."),

    Option("rgw_exit_timeout_secs", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(120)
    .set_description("RGW shutdown timeout")
    .set_long_description("Number of seconds to wait for a process before exiting unconditionally."),

    Option("rgw_get_obj_window_size", Option::TYPE_SIZE, Option::LEVEL_ADVANCED)
    .set_default(16_M)
    .set_description("RGW object read window size")
    .set_long_description("The window size in bytes for a single object read request"),

    Option("rgw_get_obj_max_req_size", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(4_M)
    .set_description("RGW object read chunk size")
    .set_long_description(
        "The maximum request size of a single object read operation sent to RADOS"),

    Option("rgw_relaxed_s3_bucket_names", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("RGW enable relaxed S3 bucket names")
    .set_long_description("RGW enable relaxed S3 bucket name rules for US region buckets."),

    Option("rgw_defer_to_bucket_acls", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Bucket ACLs override object ACLs")
    .set_long_description(
        "If not empty, a string that selects that mode of operation. 'recurse' will use "
        "bucket's ACL for the authorizaton. 'full-control' will allow users that users "
        "that have full control permission on the bucket have access to the object."),

    Option("rgw_list_buckets_max_chunk", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(1000)
    .set_description("Max number of buckets to retrieve in a single listing operation")
    .set_long_description(
        "When RGW fetches lists of user's buckets from the backend, this is the max number "
        "of entries it will try to retrieve in a single operation. Note that the backend "
        "may choose to return a smaller number of entries."),

    Option("rgw_md_log_max_shards", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(64)
    .set_description("RGW number of metadata log shards")
    .set_long_description(
        "The number of shards the RGW metadata log entries will reside in. This affects "
        "the metadata sync parallelism as a shard can only be processed by a single "
        "RGW at a time"),

    Option("rgw_num_zone_opstate_shards", Option::TYPE_INT, Option::LEVEL_DEV)
    .set_default(128)
    .set_description(""),

    Option("rgw_opstate_ratelimit_sec", Option::TYPE_INT, Option::LEVEL_DEV)
    .set_default(30)
    .set_description(""),

    Option("rgw_curl_wait_timeout_ms", Option::TYPE_INT, Option::LEVEL_DEV)
    .set_default(1000)
    .set_description(""),

    Option("rgw_curl_low_speed_limit", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(1024)
    .set_long_description(
        "It contains the average transfer speed in bytes per second that the "
        "transfer should be below during rgw_curl_low_speed_time seconds for libcurl "
        "to consider it to be too slow and abort. Set it zero to disable this."),

    Option("rgw_curl_low_speed_time", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(300)
    .set_long_description(
        "It contains the time in number seconds that the transfer speed should be below "
        "the rgw_curl_low_speed_limit for the library to consider it too slow and abort. "
        "Set it zero to disable this."),

    Option("rgw_copy_obj_progress", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(true)
    .set_description("Send progress report through copy operation")
    .set_long_description(
        "If true, RGW will send progress information when copy operation is executed. "),

    Option("rgw_copy_obj_progress_every_bytes", Option::TYPE_SIZE, Option::LEVEL_ADVANCED)
    .set_default(1_M)
    .set_description("Send copy-object progress info after these many bytes"),

    Option("rgw_obj_tombstone_cache_size", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(1000)
    .set_description("Max number of entries to keep in tombstone cache")
    .set_long_description(
        "The tombstone cache is used when doing a multi-zone data sync. RGW keeps "
        "there information about removed objects which is needed in order to prevent "
        "re-syncing of objects that were already removed."),

    Option("rgw_data_log_window", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(30)
    .set_description("Data log time window")
    .set_long_description(
        "The data log keeps information about buckets that have objectst that were "
        "modified within a specific timeframe. The sync process then knows which buckets "
        "are needed to be scanned for data sync."),

    Option("rgw_data_log_changes_size", Option::TYPE_INT, Option::LEVEL_DEV)
    .set_default(1000)
    .set_description("Max size of pending changes in data log")
    .set_long_description(
        "RGW will trigger update to the data log if the number of pending entries reached "
        "this number."),

    Option("rgw_data_log_num_shards", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(128)
    .set_description("Number of data log shards")
    .set_long_description(
        "The number of shards the RGW data log entries will reside in. This affects the "
        "data sync parallelism as a shard can only be processed by a single RGW at a time."),

    Option("rgw_data_log_obj_prefix", Option::TYPE_STR, Option::LEVEL_DEV)
    .set_default("data_log")
    .set_description(""),

    Option("rgw_replica_log_obj_prefix", Option::TYPE_STR, Option::LEVEL_DEV)
    .set_default("replica_log")
    .set_description(""),

    Option("rgw_bucket_quota_ttl", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(600)
    .set_description("Bucket quota stats cache TTL")
    .set_long_description(
        "Length of time for bucket stats to be cached within RGW instance."),

    Option("rgw_bucket_quota_soft_threshold", Option::TYPE_FLOAT, Option::LEVEL_BASIC)
    .set_default(0.95)
    .set_description("RGW quota soft threshold")
    .set_long_description(
        "Threshold from which RGW doesn't rely on cached info for quota "
        "decisions. This is done for higher accuracy of the quota mechanism at "
        "cost of performance, when getting close to the quota limit. The value "
        "configured here is the ratio between the data usage to the max usage "
        "as specified by the quota."),

    Option("rgw_bucket_quota_cache_size", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(10000)
    .set_description("RGW quota stats cache size")
    .set_long_description(
        "Maximum number of entries in the quota stats cache."),

    Option("rgw_bucket_default_quota_max_objects", Option::TYPE_INT, Option::LEVEL_BASIC)
    .set_default(-1)
    .set_description("Default quota for max objects in a bucket")
    .set_long_description(
        "The default quota configuration for max number of objects in a bucket. A "
        "negative number means 'unlimited'."),

    Option("rgw_bucket_default_quota_max_size", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(-1)
    .set_description("Default quota for total size in a bucket")
    .set_long_description(
        "The default quota configuration for total size of objects in a bucket. A "
        "negative number means 'unlimited'."),

    Option("rgw_expose_bucket", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("Send Bucket HTTP header with the response")
    .set_long_description(
        "If true, RGW will send a Bucket HTTP header with the responses. The header will "
        "contain the name of the bucket the operation happened on."),

    Option("rgw_frontends", Option::TYPE_STR, Option::LEVEL_BASIC)
    .set_default("civetweb port=7480")
    .set_description("RGW frontends configuration")
    .set_long_description(
        "A comma delimited list of frontends configuration. Each configuration contains "
        "the type of the frontend followed by an optional space delimited set of "
        "key=value config parameters."),

    Option("rgw_user_quota_bucket_sync_interval", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(180)
    .set_description("User quota bucket sync interval")
    .set_long_description(
        "Time period for accumulating modified buckets before syncing these stats."),

    Option("rgw_user_quota_sync_interval", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(1_day)
    .set_description("User quota sync interval")
    .set_long_description(
        "Time period for accumulating modified buckets before syncing entire user stats."),

    Option("rgw_user_quota_sync_idle_users", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("Should sync idle users quota")
    .set_long_description(
        "Whether stats for idle users be fully synced."),

    Option("rgw_user_quota_sync_wait_time", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(1_day)
    .set_description("User quota full-sync wait time")
    .set_long_description(
        "Minimum time between two full stats sync for non-idle users."),

    Option("rgw_user_default_quota_max_objects", Option::TYPE_INT, Option::LEVEL_BASIC)
    .set_default(-1)
    .set_description("User quota max objects")
    .set_long_description(
        "The default quota configuration for total number of objects for a single user. A "
        "negative number means 'unlimited'."),

    Option("rgw_user_default_quota_max_size", Option::TYPE_INT, Option::LEVEL_BASIC)
    .set_default(-1)
    .set_description("User quota max size")
    .set_long_description(
        "The default quota configuration for total size of objects for a single user. A "
        "negative number means 'unlimited'."),

    Option("rgw_multipart_min_part_size", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(5_M)
    .set_description("Minimum S3 multipart-upload part size")
    .set_long_description(
        "When doing a multipart upload, each part (other than the last part) should be "
        "at least this size."),

    Option("rgw_multipart_part_upload_limit", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(10000)
    .set_description("Max number of parts in multipart upload"),

    Option("rgw_max_slo_entries", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(1000)
    .set_description("Max number of entries in Swift Static Large Object manifest"),

    Option("rgw_olh_pending_timeout_sec", Option::TYPE_INT, Option::LEVEL_DEV)
    .set_default(1_hr)
    .set_description("Max time for pending OLH change to complete")
    .set_long_description(
        "OLH is a versioned object's logical head. Operations on it are journaled and "
        "as pending before completion. If an operation doesn't complete with this amount "
        "of seconds, we remove the operation from the journal."),

    Option("rgw_user_max_buckets", Option::TYPE_INT, Option::LEVEL_BASIC)
    .set_default(1000)
    .set_description("Max number of buckets per user")
    .set_long_description(
        "A user can create this many buckets. Zero means unlimmited, negative number means "
        "user cannot create any buckets (although user will retain buckets already created."),

    Option("rgw_objexp_gc_interval", Option::TYPE_UINT, Option::LEVEL_ADVANCED)
    .set_default(10_min)
    .set_description("Swift objects expirer garbage collector interval"),

    Option("rgw_objexp_hints_num_shards", Option::TYPE_UINT, Option::LEVEL_ADVANCED)
    .set_default(127)
    .set_description("Number of object expirer data shards")
    .set_long_description(
        "The number of shards the (Swift) object expirer will store its data on."),

    Option("rgw_objexp_chunk_size", Option::TYPE_UINT, Option::LEVEL_DEV)
    .set_default(100)
    .set_description(""),

    Option("rgw_enable_static_website", Option::TYPE_BOOL, Option::LEVEL_BASIC)
    .set_default(false)
    .set_description("Enable static website APIs")
    .set_long_description(
        "This configurable controls whether RGW handles the website control APIs. RGW can "
        "server static websites if s3website hostnames are configured, and unrelated to "
        "this configurable."),

    Option("rgw_log_http_headers", Option::TYPE_STR, Option::LEVEL_BASIC)
    .set_default("")
    .set_description("List of HTTP headers to log")
    .set_long_description(
        "A comma delimited list of HTTP headers to log when seen, ignores case (e.g., "
        "http_x_forwarded_for)."),

    Option("rgw_num_async_rados_threads", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(32)
    .set_description("Number of concurrent RADOS operations in multisite sync")
    .set_long_description(
        "The number of concurrent RADOS IO operations that will be triggered for handling "
        "multisite sync operations. This includes control related work, and not the actual "
        "sync operations."),

    Option("rgw_md_notify_interval_msec", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(200)
    .set_description("Length of time to aggregate metadata changes")
    .set_long_description(
        "Length of time (in milliseconds) in which the master zone aggregates all the "
        "metadata changes that occurred, before sending notifications to all the other "
        "zones."),

    Option("rgw_run_sync_thread", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(true)
    .set_description("Should run sync thread"),

    Option("rgw_sync_lease_period", Option::TYPE_INT, Option::LEVEL_DEV)
    .set_default(120)
    .set_description(""),

    Option("rgw_sync_log_trim_interval", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(1200)
    .set_description("Sync log trim interval")
    .set_long_description(
        "Time in seconds between attempts to trim sync logs."),

    Option("rgw_sync_log_trim_max_buckets", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(16)
    .set_description("Maximum number of buckets to trim per interval")
    .set_long_description("The maximum number of buckets to consider for bucket index log trimming each trim interval, regardless of the number of bucket index shards. Priority is given to buckets with the most sync activity over the last trim interval.")
    .add_see_also("rgw_sync_log_trim_interval")
    .add_see_also("rgw_sync_log_trim_min_cold_buckets")
    .add_see_also("rgw_sync_log_trim_concurrent_buckets"),

    Option("rgw_sync_log_trim_min_cold_buckets", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(4)
    .set_description("Minimum number of cold buckets to trim per interval")
    .set_long_description("Of the `rgw_sync_log_trim_max_buckets` selected for bucket index log trimming each trim interval, at least this many of them must be 'cold' buckets. These buckets are selected in order from the list of all bucket instances, to guarantee that all buckets will be visited eventually.")
    .add_see_also("rgw_sync_log_trim_interval")
    .add_see_also("rgw_sync_log_trim_max_buckets")
    .add_see_also("rgw_sync_log_trim_concurrent_buckets"),

    Option("rgw_sync_log_trim_concurrent_buckets", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(4)
    .set_description("Maximum number of buckets to trim in parallel")
    .add_see_also("rgw_sync_log_trim_interval")
    .add_see_also("rgw_sync_log_trim_max_buckets")
    .add_see_also("rgw_sync_log_trim_min_cold_buckets"),

    Option("rgw_sync_data_inject_err_probability", Option::TYPE_FLOAT, Option::LEVEL_DEV)
    .set_default(0)
    .set_description(""),

    Option("rgw_sync_meta_inject_err_probability", Option::TYPE_FLOAT, Option::LEVEL_DEV)
    .set_default(0)
    .set_description(""),

    Option("rgw_sync_trace_history_size", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(4096)
    .set_description("Sync trace history size")
    .set_long_description(
      "Maximum number of complete sync trace entries to keep."),

    Option("rgw_sync_trace_per_node_log_size", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(32)
    .set_description("Sync trace per-node log size")
    .set_long_description(
        "The number of log entries to keep per sync-trace node."),

    Option("rgw_sync_trace_servicemap_update_interval", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(10)
    .set_description("Sync-trace service-map update interval")
    .set_long_description(
        "Number of seconds between service-map updates of sync-trace events."),

    Option("rgw_period_push_interval", Option::TYPE_FLOAT, Option::LEVEL_ADVANCED)
    .set_default(2)
    .set_description("Period push interval")
    .set_long_description(
        "Number of seconds to wait before retrying 'period push' operation."),

    Option("rgw_period_push_interval_max", Option::TYPE_FLOAT, Option::LEVEL_ADVANCED)
    .set_default(30)
    .set_description("Period push maximum interval")
    .set_long_description(
        "The max number of seconds to wait before retrying 'period push' after exponential "
        "backoff."),

    Option("rgw_safe_max_objects_per_shard", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(100*1024)
    .set_description("Safe number of objects per shard")
    .set_long_description(
        "This is the max number of objects per bucket index shard that RGW considers "
        "safe. RGW will warn if it identifies a bucket where its per-shard count is "
        "higher than a percentage of this number.")
    .add_see_also("rgw_shard_warning_threshold"),

    Option("rgw_shard_warning_threshold", Option::TYPE_FLOAT, Option::LEVEL_ADVANCED)
    .set_default(90)
    .set_description("Warn about max objects per shard")
    .set_long_description(
        "Warn if number of objects per shard in a specific bucket passed this percentage "
        "of the safe number.")
    .add_see_also("rgw_safe_max_objects_per_shard"),

    Option("rgw_swift_versioning_enabled", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("Enable Swift versioning"),

    Option("rgw_swift_custom_header", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Enable swift custom header")
    .set_long_description(
        "If not empty, specifies a name of HTTP header that can include custom data. When "
        "uploading an object, if this header is passed RGW will store this header info "
        "and it will be available when listing the bucket."),

    Option("rgw_swift_need_stats", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(true)
    .set_description("Enable stats on bucket listing in Swift"),

    Option("rgw_reshard_num_logs", Option::TYPE_INT, Option::LEVEL_DEV)
    .set_default(16)
    .set_description(""),

    Option("rgw_reshard_bucket_lock_duration", Option::TYPE_INT, Option::LEVEL_DEV)
    .set_default(120)
    .set_description(""),

    Option("rgw_trust_forwarded_https", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("Trust Forwarded and X-Forwarded-Proto headers")
    .set_long_description(
        "When a proxy in front of radosgw is used for ssl termination, radosgw "
        "does not know whether incoming http connections are secure. Enable "
        "this option to trust the Forwarded and X-Forwarded-Proto headers sent "
        "by the proxy when determining whether the connection is secure. This "
        "is required for some features, such as server side encryption.")
    .add_see_also("rgw_crypt_require_ssl"),

    Option("rgw_crypt_require_ssl", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(true)
    .set_description("Requests including encryption key headers must be sent over ssl"),

    Option("rgw_crypt_default_encryption_key", Option::TYPE_STR, Option::LEVEL_DEV)
    .set_default("")
    .set_description(""),

    Option("rgw_crypt_s3_kms_encryption_keys", Option::TYPE_STR, Option::LEVEL_DEV)
    .set_default("")
    .set_description(""),

    Option("rgw_crypt_suppress_logs", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(true)
    .set_description("Suppress logs that might print client key"),

    Option("rgw_list_bucket_min_readahead", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(1000)
    .set_description("Minimum number of entries to request from rados for bucket listing"),

    Option("rgw_rest_getusage_op_compat", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("REST GetUsage request backward compatibility"),

    Option("rgw_torrent_flag", Option::TYPE_BOOL, Option::LEVEL_ADVANCED)
    .set_default(false)
    .set_description("When true, uploaded objects will calculate and store "
                     "a SHA256 hash of object data so the object can be "
                     "retrieved as a torrent file"),

    Option("rgw_torrent_tracker", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Torrent field announce and announce list"),

    Option("rgw_torrent_createby", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("torrent field created by"),

    Option("rgw_torrent_comment", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Torrent field comment"),

    Option("rgw_torrent_encoding", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("torrent field encoding"),

    Option("rgw_data_notify_interval_msec", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(200)
    .set_description("data changes notification interval to followers"),

    Option("rgw_torrent_origin", Option::TYPE_STR, Option::LEVEL_ADVANCED)
    .set_default("")
    .set_description("Torrent origin"),

    Option("rgw_torrent_sha_unit", Option::TYPE_INT, Option::LEVEL_ADVANCED)
    .set_default(512*1024)
    .set_description(""),

    Option("rgw_dynamic_resharding", Option::TYPE_BOOL, Option::LEVEL_BASIC)
    .set_default(true)
    .set_description("Enable dynamic resharding")
    .set_long_description(
        "If true, RGW will dynamicall increase the number of shards in buckets that have "
        "a high number of objects per shard.")
    .add_see_also("rgw_max_objs_per_shard"),

    Option("rgw_max_objs_per_shard", Option::TYPE_INT, Option::LEVEL_BASIC)
    .set_default(100000)
    .set_description("Max objects per shard for dynamic resharding")
    .set_long_description(
        "This is the max number of objects per bucket index shard that RGW will "
        "allow with dynamic resharding. RGW will trigger an automatic reshard operation "
        "on the bucket if it exceeds this number.")
    .add_see_also("rgw_dynamic_resharding"),

    Option("rgw_reshard_thread_interval", Option::TYPE_UINT, Option::LEVEL_ADVANCED)
    .set_default(10_min)
    .set_description(""),

    Option("rgw_cache_expiry_interval", Option::TYPE_UINT,
	   Option::LEVEL_ADVANCED)
    .set_default(15_min)
    .set_description("Number of seconds before entries in the cache are "
		     "assumed stale and re-fetched. Zero is never.")
    .add_tag("performance")
    .add_service("rgw")
    .set_long_description("The Rados Gateway stores metadata and objects in "
			  "an internal cache. This should be kept consistent "
			  "by the OSD's relaying notify events between "
			  "multiple watching RGW processes. In the event "
			  "that this notification protocol fails, bounding "
			  "the length of time that any data in the cache will "
			  "be assumed valid will ensure that any RGW instance "
			  "that falls out of sync will eventually recover. "
			  "This seems to be an issue mostly for large numbers "
			  "of RGW instances under heavy use. If you would like "
			  "to turn off cache expiry, set this value to zero."),

    Option("rgw_max_listing_results", Option::TYPE_UINT,
	   Option::LEVEL_ADVANCED)
    .set_default(1000)
    .set_min_max(1, 100000)
    .add_service("rgw")
    .set_description("Upper bound on results in listing operations, ListBucket max-keys"),
    .set_long_description("This caps the maximum permitted value for listing-like operations in RGW S3. "
			  "Affects ListBucket(max-keys), "
			  "ListBucketVersions(max-keys), "
			  "ListBucketMultiPartUploads(max-uploads), "
			  "ListMultipartUploadParts(max-parts)"),
  });
}