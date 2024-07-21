static int _php_curl_setopt(php_curl *ch, long option, zval **zvalue TSRMLS_DC) /* {{{ */
{
	CURLcode     error=CURLE_OK;

	switch (option) {
		/* Long options */
		case CURLOPT_SSL_VERIFYHOST:
			if(Z_BVAL_PP(zvalue) == 1) {
#if LIBCURL_VERSION_NUM <= 0x071c00 /* 7.28.0 */
				php_error_docref(NULL TSRMLS_CC, E_NOTICE, "CURLOPT_SSL_VERIFYHOST with value 1 is deprecated and will be removed as of libcurl 7.28.1. It is recommended to use value 2 instead");
#else
				php_error_docref(NULL TSRMLS_CC, E_NOTICE, "CURLOPT_SSL_VERIFYHOST no longer accepts the value 1, value 2 will be used instead");
				error = curl_easy_setopt(ch->cp, option, 2);
				break;
#endif
			}
		case CURLOPT_AUTOREFERER:
		case CURLOPT_BUFFERSIZE:
		case CURLOPT_CONNECTTIMEOUT:
		case CURLOPT_COOKIESESSION:
		case CURLOPT_CRLF:
		case CURLOPT_DNS_CACHE_TIMEOUT:
		case CURLOPT_DNS_USE_GLOBAL_CACHE:
		case CURLOPT_FAILONERROR:
		case CURLOPT_FILETIME:
		case CURLOPT_FORBID_REUSE:
		case CURLOPT_FRESH_CONNECT:
		case CURLOPT_FTP_USE_EPRT:
		case CURLOPT_FTP_USE_EPSV:
		case CURLOPT_HEADER:
		case CURLOPT_HTTPGET:
		case CURLOPT_HTTPPROXYTUNNEL:
		case CURLOPT_HTTP_VERSION:
		case CURLOPT_INFILESIZE:
		case CURLOPT_LOW_SPEED_LIMIT:
		case CURLOPT_LOW_SPEED_TIME:
		case CURLOPT_MAXCONNECTS:
		case CURLOPT_MAXREDIRS:
		case CURLOPT_NETRC:
		case CURLOPT_NOBODY:
		case CURLOPT_NOPROGRESS:
		case CURLOPT_NOSIGNAL:
		case CURLOPT_PORT:
		case CURLOPT_POST:
		case CURLOPT_PROXYPORT:
		case CURLOPT_PROXYTYPE:
		case CURLOPT_PUT:
		case CURLOPT_RESUME_FROM:
		case CURLOPT_SSLVERSION:
		case CURLOPT_SSL_VERIFYPEER:
		case CURLOPT_TIMECONDITION:
		case CURLOPT_TIMEOUT:
		case CURLOPT_TIMEVALUE:
		case CURLOPT_TRANSFERTEXT:
		case CURLOPT_UNRESTRICTED_AUTH:
		case CURLOPT_UPLOAD:
		case CURLOPT_VERBOSE:
#if LIBCURL_VERSION_NUM >= 0x070a06 /* Available since 7.10.6 */
		case CURLOPT_HTTPAUTH:
#endif
#if LIBCURL_VERSION_NUM >= 0x070a07 /* Available since 7.10.7 */
		case CURLOPT_FTP_CREATE_MISSING_DIRS:
		case CURLOPT_PROXYAUTH:
#endif
#if LIBCURL_VERSION_NUM >= 0x070a08 /* Available since 7.10.8 */
		case CURLOPT_FTP_RESPONSE_TIMEOUT:
		case CURLOPT_IPRESOLVE:
		case CURLOPT_MAXFILESIZE:
#endif
#if LIBCURL_VERSION_NUM >= 0x070b02 /* Available since 7.11.2 */
		case CURLOPT_TCP_NODELAY:
#endif
#if LIBCURL_VERSION_NUM >= 0x070c02 /* Available since 7.12.2 */
		case CURLOPT_FTPSSLAUTH:
#endif
#if LIBCURL_VERSION_NUM >= 0x070e01 /* Available since 7.14.1 */
		case CURLOPT_IGNORE_CONTENT_LENGTH:
#endif
#if LIBCURL_VERSION_NUM >= 0x070f00 /* Available since 7.15.0 */
		case CURLOPT_FTP_SKIP_PASV_IP:
#endif
#if LIBCURL_VERSION_NUM >= 0x070f01 /* Available since 7.15.1 */
		case CURLOPT_FTP_FILEMETHOD:
#endif
#if LIBCURL_VERSION_NUM >= 0x070f02 /* Available since 7.15.2 */
		case CURLOPT_CONNECT_ONLY:
		case CURLOPT_LOCALPORT:
		case CURLOPT_LOCALPORTRANGE:
#endif
#if LIBCURL_VERSION_NUM >= 0x071000 /* Available since 7.16.0 */
		case CURLOPT_SSL_SESSIONID_CACHE:
#endif
#if LIBCURL_VERSION_NUM >= 0x071001 /* Available since 7.16.1 */
		case CURLOPT_FTP_SSL_CCC:
		case CURLOPT_SSH_AUTH_TYPES:
#endif
#if LIBCURL_VERSION_NUM >= 0x071002 /* Available since 7.16.2 */
		case CURLOPT_CONNECTTIMEOUT_MS:
		case CURLOPT_HTTP_CONTENT_DECODING:
		case CURLOPT_HTTP_TRANSFER_DECODING:
		case CURLOPT_TIMEOUT_MS:
#endif
#if LIBCURL_VERSION_NUM >= 0x071004 /* Available since 7.16.4 */
		case CURLOPT_NEW_DIRECTORY_PERMS:
		case CURLOPT_NEW_FILE_PERMS:
#endif
#if LIBCURL_VERSION_NUM >= 0x071100 /* Available since 7.17.0 */
		case CURLOPT_USE_SSL:
#elif LIBCURL_VERSION_NUM >= 0x070b00 /* Available since 7.11.0 */
		case CURLOPT_FTP_SSL:
#endif
#if LIBCURL_VERSION_NUM >= 0x071100 /* Available since 7.17.0 */
		case CURLOPT_APPEND:
		case CURLOPT_DIRLISTONLY:
#else
		case CURLOPT_FTPAPPEND:
		case CURLOPT_FTPLISTONLY:
#endif
#if LIBCURL_VERSION_NUM >= 0x071200 /* Available since 7.18.0 */
		case CURLOPT_PROXY_TRANSFER_MODE:
#endif
#if LIBCURL_VERSION_NUM >= 0x071300 /* Available since 7.19.0 */
		case CURLOPT_ADDRESS_SCOPE:
#endif
#if LIBCURL_VERSION_NUM >  0x071301 /* Available since 7.19.1 */
		case CURLOPT_CERTINFO:
#endif
#if LIBCURL_VERSION_NUM >= 0x071304 /* Available since 7.19.4 */
		case CURLOPT_NOPROXY:
		case CURLOPT_PROTOCOLS:
		case CURLOPT_REDIR_PROTOCOLS:
		case CURLOPT_SOCKS5_GSSAPI_NEC:
		case CURLOPT_TFTP_BLKSIZE:
#endif
#if LIBCURL_VERSION_NUM >= 0x071400 /* Available since 7.20.0 */
		case CURLOPT_FTP_USE_PRET:
		case CURLOPT_RTSP_CLIENT_CSEQ:
		case CURLOPT_RTSP_REQUEST:
		case CURLOPT_RTSP_SERVER_CSEQ:
#endif
#if LIBCURL_VERSION_NUM >= 0x071500 /* Available since 7.21.0 */
		case CURLOPT_WILDCARDMATCH:
#endif
#if LIBCURL_VERSION_NUM >= 0x071504 /* Available since 7.21.4 */
		case CURLOPT_TLSAUTH_TYPE:
#endif
#if LIBCURL_VERSION_NUM >= 0x071600 /* Available since 7.22.0 */
		case CURLOPT_GSSAPI_DELEGATION:
#endif
#if LIBCURL_VERSION_NUM >= 0x071800 /* Available since 7.24.0 */
		case CURLOPT_ACCEPTTIMEOUT_MS:
#endif
#if LIBCURL_VERSION_NUM >= 0x071900 /* Available since 7.25.0 */
		case CURLOPT_SSL_OPTIONS:
		case CURLOPT_TCP_KEEPALIVE:
		case CURLOPT_TCP_KEEPIDLE:
		case CURLOPT_TCP_KEEPINTVL:
#endif
#if CURLOPT_MUTE != 0
		case CURLOPT_MUTE:
#endif
			convert_to_long_ex(zvalue);
#if LIBCURL_VERSION_NUM >= 0x71304
			if ((option == CURLOPT_PROTOCOLS || option == CURLOPT_REDIR_PROTOCOLS) &&
				(PG(open_basedir) && *PG(open_basedir)) && (Z_LVAL_PP(zvalue) & CURLPROTO_FILE)) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "CURLPROTO_FILE cannot be activated when an open_basedir is set");
					return 1;
			}
#endif
			error = curl_easy_setopt(ch->cp, option, Z_LVAL_PP(zvalue));
			break;
		case CURLOPT_SAFE_UPLOAD:
			convert_to_long_ex(zvalue);
			ch->safe_upload = (Z_LVAL_PP(zvalue) != 0);
			break;

		/* String options */
		case CURLOPT_CAINFO:
		case CURLOPT_CAPATH:
		case CURLOPT_COOKIE:
		case CURLOPT_EGDSOCKET:
		case CURLOPT_INTERFACE:
		case CURLOPT_PROXY:
		case CURLOPT_PROXYUSERPWD:
		case CURLOPT_REFERER:
		case CURLOPT_SSLCERTTYPE:
		case CURLOPT_SSLENGINE:
		case CURLOPT_SSLENGINE_DEFAULT:
		case CURLOPT_SSLKEY:
		case CURLOPT_SSLKEYPASSWD:
		case CURLOPT_SSLKEYTYPE:
		case CURLOPT_SSL_CIPHER_LIST:
		case CURLOPT_USERAGENT:
		case CURLOPT_USERPWD:
#if LIBCURL_VERSION_NUM >= 0x070e01 /* Available since 7.14.1 */
		case CURLOPT_COOKIELIST:
#endif
#if LIBCURL_VERSION_NUM >= 0x070f05 /* Available since 7.15.5 */
		case CURLOPT_FTP_ALTERNATIVE_TO_USER:
#endif
#if LIBCURL_VERSION_NUM >= 0x071101 /* Available since 7.17.1 */
		case CURLOPT_SSH_HOST_PUBLIC_KEY_MD5:
#endif
#if LIBCURL_VERSION_NUM >= 0x071301 /* Available since 7.19.1 */
		case CURLOPT_PASSWORD:
		case CURLOPT_PROXYPASSWORD:
		case CURLOPT_PROXYUSERNAME:
		case CURLOPT_USERNAME:
#endif
#if LIBCURL_VERSION_NUM >= 0x071304 /* Available since 7.19.4 */
		case CURLOPT_SOCKS5_GSSAPI_SERVICE:
#endif
#if LIBCURL_VERSION_NUM >= 0x071400 /* Available since 7.20.0 */
		case CURLOPT_MAIL_FROM:
		case CURLOPT_RTSP_STREAM_URI:
		case CURLOPT_RTSP_TRANSPORT:
#endif
#if LIBCURL_VERSION_NUM >= 0x071504 /* Available since 7.21.4 */
		case CURLOPT_TLSAUTH_PASSWORD:
		case CURLOPT_TLSAUTH_USERNAME:
#endif
#if LIBCURL_VERSION_NUM >= 0x071506 /* Available since 7.21.6 */
		case CURLOPT_ACCEPT_ENCODING:
		case CURLOPT_TRANSFER_ENCODING:
#else
		case CURLOPT_ENCODING:
#endif
#if LIBCURL_VERSION_NUM >= 0x071800 /* Available since 7.24.0 */
		case CURLOPT_DNS_SERVERS:
#endif
#if LIBCURL_VERSION_NUM >= 0x071900 /* Available since 7.25.0 */
		case CURLOPT_MAIL_AUTH:
#endif
		{
			convert_to_string_ex(zvalue);
			return php_curl_option_str(ch, option, Z_STRVAL_PP(zvalue), Z_STRLEN_PP(zvalue), 0 TSRMLS_CC);
		}

		/* Curl nullable string options */
		case CURLOPT_CUSTOMREQUEST:
		case CURLOPT_FTPPORT:
		case CURLOPT_RANGE:
#if LIBCURL_VERSION_NUM >= 0x070d00 /* Available since 7.13.0 */
		case CURLOPT_FTP_ACCOUNT:
#endif
#if LIBCURL_VERSION_NUM >= 0x071400 /* Available since 7.20.0 */
		case CURLOPT_RTSP_SESSION_ID:
#endif
#if LIBCURL_VERSION_NUM >= 0x071004 /* Available since 7.16.4 */
		case CURLOPT_KRBLEVEL:
#else
		case CURLOPT_KRB4LEVEL:
#endif
		{
			if (Z_TYPE_PP(zvalue) == IS_NULL) {
				error = curl_easy_setopt(ch->cp, option, NULL);
			} else {
				convert_to_string_ex(zvalue);
				return php_curl_option_str(ch, option, Z_STRVAL_PP(zvalue), Z_STRLEN_PP(zvalue), 0 TSRMLS_CC);
			}
			break;
		}

		/* Curl private option */
		case CURLOPT_PRIVATE:
			convert_to_string_ex(zvalue);
			return php_curl_option_str(ch, option, Z_STRVAL_PP(zvalue), Z_STRLEN_PP(zvalue), 1 TSRMLS_CC);

		/* Curl url option */
		case CURLOPT_URL:
			convert_to_string_ex(zvalue);
			return php_curl_option_url(ch, Z_STRVAL_PP(zvalue), Z_STRLEN_PP(zvalue) TSRMLS_CC);

		/* Curl file handle options */
		case CURLOPT_FILE:
		case CURLOPT_INFILE:
		case CURLOPT_STDERR:
		case CURLOPT_WRITEHEADER: {
			FILE *fp = NULL;
			int type;
			void *what = NULL;

			if (Z_TYPE_PP(zvalue) != IS_NULL) {
				what = zend_fetch_resource(zvalue TSRMLS_CC, -1, "File-Handle", &type, 1, php_file_le_stream(), php_file_le_pstream());
				if (!what) {
					return FAILURE;
				}

				if (FAILURE == php_stream_cast((php_stream *) what, PHP_STREAM_AS_STDIO, (void *) &fp, REPORT_ERRORS)) {
					return FAILURE;
				}

				if (!fp) {
					return FAILURE;
				}
			}

			error = CURLE_OK;
			switch (option) {
				case CURLOPT_FILE:
					if (!what) {
						if (ch->handlers->write->stream) {
							Z_DELREF_P(ch->handlers->write->stream);
							ch->handlers->write->stream = NULL;
						}
						ch->handlers->write->fp = NULL;
						ch->handlers->write->method = PHP_CURL_STDOUT;
					} else if (((php_stream *) what)->mode[0] != 'r' || ((php_stream *) what)->mode[1] == '+') {
						if (ch->handlers->write->stream) {
							Z_DELREF_P(ch->handlers->write->stream);
						}
						Z_ADDREF_PP(zvalue);
						ch->handlers->write->fp = fp;
						ch->handlers->write->method = PHP_CURL_FILE;
						ch->handlers->write->stream = *zvalue;
					} else {
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "the provided file handle is not writable");
						return FAILURE;
					}
					break;
				case CURLOPT_WRITEHEADER:
					if (!what) {
						if (ch->handlers->write_header->stream) {
							Z_DELREF_P(ch->handlers->write_header->stream);
							ch->handlers->write_header->stream = NULL;
						}
						ch->handlers->write_header->fp = NULL;
						ch->handlers->write_header->method = PHP_CURL_IGNORE;
					} else if (((php_stream *) what)->mode[0] != 'r' || ((php_stream *) what)->mode[1] == '+') {
						if (ch->handlers->write_header->stream) {
							Z_DELREF_P(ch->handlers->write_header->stream);
						}
						Z_ADDREF_PP(zvalue);
						ch->handlers->write_header->fp = fp;
						ch->handlers->write_header->method = PHP_CURL_FILE;
						ch->handlers->write_header->stream = *zvalue;
					} else {
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "the provided file handle is not writable");
						return FAILURE;
					}
					break;
				case CURLOPT_INFILE:
					if (!what) {
						if (ch->handlers->read->stream) {
							Z_DELREF_P(ch->handlers->read->stream);
							ch->handlers->read->stream = NULL;
						}
						ch->handlers->read->fp = NULL;
						ch->handlers->read->fd = 0;
					} else {
						if (ch->handlers->read->stream) {
							Z_DELREF_P(ch->handlers->read->stream);
						}
						Z_ADDREF_PP(zvalue);
						ch->handlers->read->fp = fp;
						ch->handlers->read->fd = Z_LVAL_PP(zvalue);
						ch->handlers->read->stream = *zvalue;
					}
					break;
				case CURLOPT_STDERR:
					if (!what) {
						if (ch->handlers->std_err) {
							zval_ptr_dtor(&ch->handlers->std_err);
							ch->handlers->std_err = NULL;
						}
					} else if (((php_stream *) what)->mode[0] != 'r' || ((php_stream *) what)->mode[1] == '+') {
						if (ch->handlers->std_err) {
							zval_ptr_dtor(&ch->handlers->std_err);
						}
						zval_add_ref(zvalue);
						ch->handlers->std_err = *zvalue;
					} else {
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "the provided file handle is not writable");
						return FAILURE;
					}
					/* break omitted intentionally */
				default:
					error = curl_easy_setopt(ch->cp, option, fp);
					break;
			}
			break;
		}

		/* Curl linked list options */
		case CURLOPT_HTTP200ALIASES:
		case CURLOPT_HTTPHEADER:
		case CURLOPT_POSTQUOTE:
		case CURLOPT_PREQUOTE:
		case CURLOPT_QUOTE:
		case CURLOPT_TELNETOPTIONS:
#if LIBCURL_VERSION_NUM >= 0x071400 /* Available since 7.20.0 */
		case CURLOPT_MAIL_RCPT:
#endif
#if LIBCURL_VERSION_NUM >= 0x071503 /* Available since 7.21.3 */
		case CURLOPT_RESOLVE:
#endif
		{
			zval              **current;
			HashTable          *ph;
			struct curl_slist  *slist = NULL;

			ph = HASH_OF(*zvalue);
			if (!ph) {
				char *name = NULL;
				switch (option) {
					case CURLOPT_HTTPHEADER:
						name = "CURLOPT_HTTPHEADER";
						break;
					case CURLOPT_QUOTE:
						name = "CURLOPT_QUOTE";
						break;
					case CURLOPT_HTTP200ALIASES:
						name = "CURLOPT_HTTP200ALIASES";
						break;
					case CURLOPT_POSTQUOTE:
						name = "CURLOPT_POSTQUOTE";
						break;
					case CURLOPT_PREQUOTE:
						name = "CURLOPT_PREQUOTE";
						break;
					case CURLOPT_TELNETOPTIONS:
						name = "CURLOPT_TELNETOPTIONS";
						break;
#if LIBCURL_VERSION_NUM >= 0x071400 /* Available since 7.20.0 */
					case CURLOPT_MAIL_RCPT:
						name = "CURLOPT_MAIL_RCPT";
						break;
#endif
#if LIBCURL_VERSION_NUM >= 0x071503 /* Available since 7.21.3 */
					case CURLOPT_RESOLVE:
						name = "CURLOPT_RESOLVE";
						break;
#endif
				}
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "You must pass either an object or an array with the %s argument", name);
				return FAILURE;
			}

			for (zend_hash_internal_pointer_reset(ph);
				 zend_hash_get_current_data(ph, (void **) &current) == SUCCESS;
				 zend_hash_move_forward(ph)
			) {
				SEPARATE_ZVAL(current);
				convert_to_string_ex(current);

				slist = curl_slist_append(slist, Z_STRVAL_PP(current));
				if (!slist) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Could not build curl_slist");
					return 1;
				}
			}
			zend_hash_index_update(ch->to_free->slist, (ulong) option, &slist, sizeof(struct curl_slist *), NULL);

			error = curl_easy_setopt(ch->cp, option, slist);

			break;
		}

		case CURLOPT_BINARYTRANSFER:
			/* Do nothing, just backward compatibility */
			break;

		case CURLOPT_FOLLOWLOCATION:
			convert_to_long_ex(zvalue);
#if LIBCURL_VERSION_NUM < 0x071304
			if (PG(open_basedir) && *PG(open_basedir)) {
				if (Z_LVAL_PP(zvalue) != 0) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "CURLOPT_FOLLOWLOCATION cannot be activated when an open_basedir is set");
					return FAILURE;
				}
			}
#endif
			error = curl_easy_setopt(ch->cp, option, Z_LVAL_PP(zvalue));
			break;

		case CURLOPT_HEADERFUNCTION:
			if (ch->handlers->write_header->func_name) {
				zval_ptr_dtor(&ch->handlers->write_header->func_name);
				ch->handlers->write_header->fci_cache = empty_fcall_info_cache;
			}
			zval_add_ref(zvalue);
			ch->handlers->write_header->func_name = *zvalue;
			ch->handlers->write_header->method = PHP_CURL_USER;
			break;

		case CURLOPT_POSTFIELDS:
			if (Z_TYPE_PP(zvalue) == IS_ARRAY || Z_TYPE_PP(zvalue) == IS_OBJECT) {
				zval            **current;
				HashTable        *postfields;
				struct HttpPost  *first = NULL;
				struct HttpPost  *last  = NULL;

				postfields = HASH_OF(*zvalue);
				if (!postfields) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Couldn't get HashTable in CURLOPT_POSTFIELDS");
					return FAILURE;
				}

				for (zend_hash_internal_pointer_reset(postfields);
					 zend_hash_get_current_data(postfields, (void **) &current) == SUCCESS;
					 zend_hash_move_forward(postfields)
				) {
					char  *postval;
					char  *string_key = NULL;
					uint   string_key_len;
					ulong  num_key;
					int    numeric_key;

					zend_hash_get_current_key_ex(postfields, &string_key, &string_key_len, &num_key, 0, NULL);

					/* Pretend we have a string_key here */
					if(!string_key) {
						spprintf(&string_key, 0, "%ld", num_key);
						string_key_len = strlen(string_key)+1;
						numeric_key = 1;
					} else {
						numeric_key = 0;
					}

					if(Z_TYPE_PP(current) == IS_OBJECT && instanceof_function(Z_OBJCE_PP(current), curl_CURLFile_class TSRMLS_CC)) {
						/* new-style file upload */
						zval *prop;
						char *type = NULL, *filename = NULL;

						prop = zend_read_property(curl_CURLFile_class, *current, "name", sizeof("name")-1, 0 TSRMLS_CC);
						if(Z_TYPE_P(prop) != IS_STRING) {
							php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid filename for key %s", string_key);
						} else {
							postval = Z_STRVAL_P(prop);

							if (php_check_open_basedir(postval TSRMLS_CC)) {
								return 1;
							}

							prop = zend_read_property(curl_CURLFile_class, *current, "mime", sizeof("mime")-1, 0 TSRMLS_CC);
							if(Z_TYPE_P(prop) == IS_STRING && Z_STRLEN_P(prop) > 0) {
								type = Z_STRVAL_P(prop);
							}
							prop = zend_read_property(curl_CURLFile_class, *current, "postname", sizeof("postname")-1, 0 TSRMLS_CC);
							if(Z_TYPE_P(prop) == IS_STRING && Z_STRLEN_P(prop) > 0) {
								filename = Z_STRVAL_P(prop);
							}
							error = curl_formadd(&first, &last,
											CURLFORM_COPYNAME, string_key,
											CURLFORM_NAMELENGTH, (long)string_key_len - 1,
											CURLFORM_FILENAME, filename ? filename : postval,
											CURLFORM_CONTENTTYPE, type ? type : "application/octet-stream",
											CURLFORM_FILE, postval,
											CURLFORM_END);
						}

						if (numeric_key) {
							efree(string_key);
						}
						continue;
					}

					SEPARATE_ZVAL(current);
					convert_to_string_ex(current);

					postval = Z_STRVAL_PP(current);

					/* The arguments after _NAMELENGTH and _CONTENTSLENGTH
					 * must be explicitly cast to long in curl_formadd
					 * use since curl needs a long not an int. */
					if (!ch->safe_upload && *postval == '@') {
						char *type, *filename;
						++postval;

						php_error_docref("curl.curlfile" TSRMLS_CC, E_DEPRECATED, "The usage of the @filename API for file uploading is deprecated. Please use the CURLFile class instead");

						if ((type = php_memnstr(postval, ";type=", sizeof(";type=") - 1, postval + Z_STRLEN_PP(current)))) {
							*type = '\0';
						}
						if ((filename = php_memnstr(postval, ";filename=", sizeof(";filename=") - 1, postval + Z_STRLEN_PP(current)))) {
							*filename = '\0';
						}
						/* open_basedir check */
						if (php_check_open_basedir(postval TSRMLS_CC)) {
							return FAILURE;
						}
						error = curl_formadd(&first, &last,
										CURLFORM_COPYNAME, string_key,
										CURLFORM_NAMELENGTH, (long)string_key_len - 1,
										CURLFORM_FILENAME, filename ? filename + sizeof(";filename=") - 1 : postval,
										CURLFORM_CONTENTTYPE, type ? type + sizeof(";type=") - 1 : "application/octet-stream",
										CURLFORM_FILE, postval,
										CURLFORM_END);
						if (type) {
							*type = ';';
						}
						if (filename) {
							*filename = ';';
						}
					} else {
						error = curl_formadd(&first, &last,
											 CURLFORM_COPYNAME, string_key,
											 CURLFORM_NAMELENGTH, (long)string_key_len - 1,
											 CURLFORM_COPYCONTENTS, postval,
											 CURLFORM_CONTENTSLENGTH, (long)Z_STRLEN_PP(current),
											 CURLFORM_END);
					}

					if (numeric_key) {
						efree(string_key);
					}
				}

				SAVE_CURL_ERROR(ch, error);
				if (error != CURLE_OK) {
					return FAILURE;
				}

				if (Z_REFCOUNT_P(ch->clone) <= 1) {
					zend_llist_clean(&ch->to_free->post);
				}
				zend_llist_add_element(&ch->to_free->post, &first);
				error = curl_easy_setopt(ch->cp, CURLOPT_HTTPPOST, first);

			} else {
#if LIBCURL_VERSION_NUM >= 0x071101
				convert_to_string_ex(zvalue);
				/* with curl 7.17.0 and later, we can use COPYPOSTFIELDS, but we have to provide size before */
				error = curl_easy_setopt(ch->cp, CURLOPT_POSTFIELDSIZE, Z_STRLEN_PP(zvalue));
				error = curl_easy_setopt(ch->cp, CURLOPT_COPYPOSTFIELDS, Z_STRVAL_PP(zvalue));
#else
				char *post = NULL;

				convert_to_string_ex(zvalue);
				post = estrndup(Z_STRVAL_PP(zvalue), Z_STRLEN_PP(zvalue));
				zend_llist_add_element(&ch->to_free->str, &post);

				curl_easy_setopt(ch->cp, CURLOPT_POSTFIELDS, post);
				error = curl_easy_setopt(ch->cp, CURLOPT_POSTFIELDSIZE, Z_STRLEN_PP(zvalue));
#endif
			}
			break;

		case CURLOPT_PROGRESSFUNCTION:
			curl_easy_setopt(ch->cp, CURLOPT_PROGRESSFUNCTION,	curl_progress);
			curl_easy_setopt(ch->cp, CURLOPT_PROGRESSDATA, ch);
			if (ch->handlers->progress == NULL) {
				ch->handlers->progress = ecalloc(1, sizeof(php_curl_progress));
			} else if (ch->handlers->progress->func_name) {
				zval_ptr_dtor(&ch->handlers->progress->func_name);
				ch->handlers->progress->fci_cache = empty_fcall_info_cache;
			}
			zval_add_ref(zvalue);
			ch->handlers->progress->func_name = *zvalue;
			ch->handlers->progress->method = PHP_CURL_USER;
			break;

		case CURLOPT_READFUNCTION:
			if (ch->handlers->read->func_name) {
				zval_ptr_dtor(&ch->handlers->read->func_name);
				ch->handlers->read->fci_cache = empty_fcall_info_cache;
			}
			zval_add_ref(zvalue);
			ch->handlers->read->func_name = *zvalue;
			ch->handlers->read->method = PHP_CURL_USER;
			break;

		case CURLOPT_RETURNTRANSFER:
			convert_to_long_ex(zvalue);
			if (Z_LVAL_PP(zvalue)) {
				ch->handlers->write->method = PHP_CURL_RETURN;
			} else {
				ch->handlers->write->method = PHP_CURL_STDOUT;
			}
			break;

		case CURLOPT_WRITEFUNCTION:
			if (ch->handlers->write->func_name) {
				zval_ptr_dtor(&ch->handlers->write->func_name);
				ch->handlers->write->fci_cache = empty_fcall_info_cache;
			}
			zval_add_ref(zvalue);
			ch->handlers->write->func_name = *zvalue;
			ch->handlers->write->method = PHP_CURL_USER;
			break;

#if LIBCURL_VERSION_NUM >= 0x070f05 /* Available since 7.15.5 */
		case CURLOPT_MAX_RECV_SPEED_LARGE:
		case CURLOPT_MAX_SEND_SPEED_LARGE:
			convert_to_long_ex(zvalue);
			error = curl_easy_setopt(ch->cp, option, (curl_off_t)Z_LVAL_PP(zvalue));
			break;
#endif

#if LIBCURL_VERSION_NUM >= 0x071301 /* Available since 7.19.1 */
		case CURLOPT_POSTREDIR:
			convert_to_long_ex(zvalue);
			error = curl_easy_setopt(ch->cp, CURLOPT_POSTREDIR, Z_LVAL_PP(zvalue) & CURL_REDIR_POST_ALL);
			break;
#endif

#if CURLOPT_PASSWDFUNCTION != 0
		case CURLOPT_PASSWDFUNCTION:
			if (ch->handlers->passwd) {
				zval_ptr_dtor(&ch->handlers->passwd);
			}
			zval_add_ref(zvalue);
			ch->handlers->passwd = *zvalue;
			error = curl_easy_setopt(ch->cp, CURLOPT_PASSWDFUNCTION, curl_passwd);
			error = curl_easy_setopt(ch->cp, CURLOPT_PASSWDDATA,     (void *) ch);
			break;
#endif

		/* the following options deal with files, therefore the open_basedir check
		 * is required.
		 */
		case CURLOPT_COOKIEFILE:
		case CURLOPT_COOKIEJAR:
		case CURLOPT_RANDOM_FILE:
		case CURLOPT_SSLCERT:
#if LIBCURL_VERSION_NUM >= 0x070b00 /* Available since 7.11.0 */
		case CURLOPT_NETRC_FILE:
#endif
#if LIBCURL_VERSION_NUM >= 0x071001 /* Available since 7.16.1 */
		case CURLOPT_SSH_PRIVATE_KEYFILE:
		case CURLOPT_SSH_PUBLIC_KEYFILE:
#endif
#if LIBCURL_VERSION_NUM >= 0x071300 /* Available since 7.19.0 */
		case CURLOPT_CRLFILE:
		case CURLOPT_ISSUERCERT:
#endif
#if LIBCURL_VERSION_NUM >= 0x071306 /* Available since 7.19.6 */
		case CURLOPT_SSH_KNOWNHOSTS:
#endif
		{
			convert_to_string_ex(zvalue);

			if (Z_STRLEN_PP(zvalue) && php_check_open_basedir(Z_STRVAL_PP(zvalue) TSRMLS_CC)) {
				return FAILURE;
			}

			return php_curl_option_str(ch, option, Z_STRVAL_PP(zvalue), Z_STRLEN_PP(zvalue), 0 TSRMLS_CC);
		}

		case CURLINFO_HEADER_OUT:
			convert_to_long_ex(zvalue);
			if (Z_LVAL_PP(zvalue) == 1) {
				curl_easy_setopt(ch->cp, CURLOPT_DEBUGFUNCTION, curl_debug);
				curl_easy_setopt(ch->cp, CURLOPT_DEBUGDATA, (void *)ch);
				curl_easy_setopt(ch->cp, CURLOPT_VERBOSE, 1);
			} else {
				curl_easy_setopt(ch->cp, CURLOPT_DEBUGFUNCTION, NULL);
				curl_easy_setopt(ch->cp, CURLOPT_DEBUGDATA, NULL);
				curl_easy_setopt(ch->cp, CURLOPT_VERBOSE, 0);
			}
			break;

		case CURLOPT_SHARE:
			{
				php_curlsh *sh = NULL;
				ZEND_FETCH_RESOURCE_NO_RETURN(sh, php_curlsh *, zvalue, -1, le_curl_share_handle_name, le_curl_share_handle);
				if (sh) {
					curl_easy_setopt(ch->cp, CURLOPT_SHARE, sh->share);
				}
			}
			break;

#if LIBCURL_VERSION_NUM >= 0x071500 /* Available since 7.21.0 */
		case CURLOPT_FNMATCH_FUNCTION:
			curl_easy_setopt(ch->cp, CURLOPT_FNMATCH_FUNCTION, curl_fnmatch);
			curl_easy_setopt(ch->cp, CURLOPT_FNMATCH_DATA, ch);
			if (ch->handlers->fnmatch == NULL) {
				ch->handlers->fnmatch = ecalloc(1, sizeof(php_curl_fnmatch));
			} else if (ch->handlers->fnmatch->func_name) {
				zval_ptr_dtor(&ch->handlers->fnmatch->func_name);
				ch->handlers->fnmatch->fci_cache = empty_fcall_info_cache;
			}
			zval_add_ref(zvalue);
			ch->handlers->fnmatch->func_name = *zvalue;
			ch->handlers->fnmatch->method = PHP_CURL_USER;
			break;
#endif

	}

	SAVE_CURL_ERROR(ch, error);
	if (error != CURLE_OK) {
		return FAILURE;
	} else {
		return SUCCESS;
	}
}