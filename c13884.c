int callback_static_compressed_inmemory_website (const struct _u_request * request, struct _u_response * response, void * user_data) {
  struct _u_compressed_inmemory_website_config * config = (struct _u_compressed_inmemory_website_config *)user_data;
  char ** accept_list = NULL;
  int ret = U_CALLBACK_CONTINUE, compress_mode = U_COMPRESS_NONE, res;
  z_stream defstream;
  unsigned char * file_content, * file_content_orig = NULL;
  size_t length, read_length, offset, data_zip_len = 0;
  FILE * f;
  char * file_requested, * file_path, * url_dup_save, * data_zip = NULL;
  const char * content_type;

  /*
   * Comment this if statement if you don't access static files url from root dir, like /app
   */
  if (request->callback_position > 0) {
    return U_CALLBACK_IGNORE;
  } else {
    file_requested = o_strdup(request->http_url);
    url_dup_save = file_requested;

    file_requested += o_strlen((config->url_prefix));
    while (file_requested[0] == '/') {
      file_requested++;
    }

    if (strchr(file_requested, '#') != NULL) {
      *strchr(file_requested, '#') = '\0';
    }

    if (strchr(file_requested, '?') != NULL) {
      *strchr(file_requested, '?') = '\0';
    }

    if (file_requested == NULL || o_strnullempty(file_requested) || 0 == o_strcmp("/", file_requested)) {
      o_free(url_dup_save);
      url_dup_save = file_requested = o_strdup("index.html");
    }

    if (!u_map_has_key_case(response->map_header, U_CONTENT_HEADER)) {
      if (split_string(u_map_get_case(request->map_header, U_ACCEPT_HEADER), ",", &accept_list)) {
        if (config->allow_gzip && string_array_has_trimmed_value((const char **)accept_list, U_ACCEPT_GZIP)) {
          compress_mode = U_COMPRESS_GZIP;
        } else if (config->allow_deflate && string_array_has_trimmed_value((const char **)accept_list, U_ACCEPT_DEFLATE)) {
          compress_mode = U_COMPRESS_DEFL;
        }


        if (compress_mode != U_COMPRESS_NONE) {
          if (compress_mode == U_COMPRESS_GZIP && config->allow_cache_compressed && u_map_has_key(&config->gzip_files, file_requested)) {
            ulfius_set_binary_body_response(response, 200, u_map_get(&config->gzip_files, file_requested), u_map_get_length(&config->gzip_files, file_requested));
            u_map_put(response->map_header, U_CONTENT_HEADER, U_ACCEPT_GZIP);
            
            content_type = u_map_get_case(&config->mime_types, get_filename_ext(file_requested));
            if (content_type == NULL) {
              content_type = u_map_get(&config->mime_types, "*");
            }
            u_map_put(response->map_header, "Content-Type", content_type);
            u_map_copy_into(response->map_header, &config->map_header);
          } else if (compress_mode == U_COMPRESS_DEFL && config->allow_cache_compressed && u_map_has_key(&config->deflate_files, file_requested)) {
            ulfius_set_binary_body_response(response, 200, u_map_get(&config->deflate_files, file_requested), u_map_get_length(&config->deflate_files, file_requested));
            u_map_put(response->map_header, U_CONTENT_HEADER, U_ACCEPT_DEFLATE);
            
            content_type = u_map_get_case(&config->mime_types, get_filename_ext(file_requested));
            if (content_type == NULL) {
              content_type = u_map_get(&config->mime_types, "*");
            }
            u_map_put(response->map_header, "Content-Type", content_type);
            u_map_copy_into(response->map_header, &config->map_header);
          } else {
            file_path = msprintf("%s/%s", ((struct _u_compressed_inmemory_website_config *)user_data)->files_path, file_requested);

            if (!pthread_mutex_lock(&config->lock)) {
              f = fopen (file_path, "rb");
              if (f) {
                content_type = u_map_get_case(&config->mime_types, get_filename_ext(file_requested));
                if (content_type == NULL) {
                  content_type = u_map_get(&config->mime_types, "*");
                  y_log_message(Y_LOG_LEVEL_WARNING, "Static File Server - Unknown mime type for extension %s", get_filename_ext(file_requested));
                }
                if (!string_array_has_value((const char **)config->mime_types_compressed, content_type)) {
                  compress_mode = U_COMPRESS_NONE;
                }

                u_map_put(response->map_header, "Content-Type", content_type);
                u_map_copy_into(response->map_header, &config->map_header);
                
                fseek (f, 0, SEEK_END);
                offset = length = ftell (f);
                fseek (f, 0, SEEK_SET);

                if (length) {
                  if ((file_content_orig = file_content = o_malloc(length)) != NULL && (data_zip = o_malloc((2*length)+20)) != NULL) {
                    defstream.zalloc = u_zalloc;
                    defstream.zfree = u_zfree;
                    defstream.opaque = Z_NULL;
                    defstream.avail_in = (uInt)length;
                    defstream.next_in = (Bytef *)file_content;
                    while ((read_length = fread(file_content, sizeof(char), offset, f))) {
                      file_content += read_length;
                      offset -= read_length;
                    }

                    if (compress_mode == U_COMPRESS_GZIP) {
                      if (deflateInit2(&defstream,
                                       Z_DEFAULT_COMPRESSION,
                                       Z_DEFLATED,
                                       U_GZIP_WINDOW_BITS | U_GZIP_ENCODING,
                                       8,
                                       Z_DEFAULT_STRATEGY) != Z_OK) {
                        y_log_message(Y_LOG_LEVEL_ERROR, "callback_static_compressed_inmemory_website - Error deflateInit (gzip)");
                        ret = U_CALLBACK_ERROR;
                      }
                    } else {
                      if (deflateInit(&defstream, Z_BEST_COMPRESSION) != Z_OK) {
                        y_log_message(Y_LOG_LEVEL_ERROR, "callback_static_compressed_inmemory_website - Error deflateInit (deflate)");
                        ret = U_CALLBACK_ERROR;
                      }
                    }
                    if (ret == U_CALLBACK_CONTINUE) {
                      do {
                        if ((data_zip = o_realloc(data_zip, data_zip_len+_U_W_BLOCK_SIZE)) != NULL) {
                          defstream.avail_out = _U_W_BLOCK_SIZE;
                          defstream.next_out = ((Bytef *)data_zip)+data_zip_len;
                          switch ((res = deflate(&defstream, Z_FINISH))) {
                            case Z_OK:
                            case Z_STREAM_END:
                            case Z_BUF_ERROR:
                              break;
                            default:
                              y_log_message(Y_LOG_LEVEL_ERROR, "callback_static_compressed_inmemory_website - Error deflate %d", res);
                              ret = U_CALLBACK_ERROR;
                              break;
                          }
                          data_zip_len += _U_W_BLOCK_SIZE - defstream.avail_out;
                        } else {
                          y_log_message(Y_LOG_LEVEL_ERROR, "callback_static_compressed_inmemory_website - Error allocating resources for data_zip");
                          ret = U_CALLBACK_ERROR;
                        }
                      } while (U_CALLBACK_CONTINUE == ret && defstream.avail_out == 0);

                      if (ret == U_CALLBACK_CONTINUE) {
                        if (compress_mode == U_COMPRESS_GZIP) {
                          if (config->allow_cache_compressed) {
                            u_map_put_binary(&config->gzip_files, file_requested, data_zip, 0, defstream.total_out);
                          }
                          ulfius_set_binary_body_response(response, 200, u_map_get(&config->gzip_files, file_requested), u_map_get_length(&config->gzip_files, file_requested));
                        } else {
                          if (config->allow_cache_compressed) {
                            u_map_put_binary(&config->deflate_files, file_requested, data_zip, 0, defstream.total_out);
                          }
                          ulfius_set_binary_body_response(response, 200, u_map_get(&config->deflate_files, file_requested), u_map_get_length(&config->deflate_files, file_requested));
                        }
                        u_map_put(response->map_header, U_CONTENT_HEADER, compress_mode==U_COMPRESS_GZIP?U_ACCEPT_GZIP:U_ACCEPT_DEFLATE);
                      }
                    }
                    deflateEnd(&defstream);
                    o_free(data_zip);
                  } else {
                    y_log_message(Y_LOG_LEVEL_ERROR, "callback_static_compressed_inmemory_website - Error allocating resource for file_content or data_zip");
                    ret = U_CALLBACK_ERROR;
                  }
                  o_free(file_content_orig);
                }
                fclose(f);
              } else {
                if (((struct _u_compressed_inmemory_website_config *)user_data)->redirect_on_404 == NULL) {
                  ret = U_CALLBACK_IGNORE;
                } else {
                  ulfius_add_header_to_response(response, "Location", ((struct _u_compressed_inmemory_website_config *)user_data)->redirect_on_404);
                  response->status = 302;
                }
              }
              pthread_mutex_unlock(&config->lock);
            } else {
              y_log_message(Y_LOG_LEVEL_ERROR, "callback_static_compressed_inmemory_website - Error pthread_lock_mutex");
              ret = U_CALLBACK_ERROR;
            }
            o_free(file_path);
          }
        } else {
          ret = callback_static_file_uncompressed(request, response, user_data);
        }
        free_string_array(accept_list);
      }
    }
    o_free(url_dup_save);
  }

  return ret;
}