static void php_cli_server_client_populate_request_info(const php_cli_server_client *client, sapi_request_info *request_info) /* {{{ */
{
	char **val;

	request_info->request_method = php_http_method_str(client->request.request_method);
	request_info->proto_num = client->request.protocol_version;
	request_info->request_uri = client->request.request_uri;
	request_info->path_translated = client->request.path_translated;
	request_info->query_string = client->request.query_string;
	request_info->post_data = client->request.content;
	request_info->content_length = request_info->post_data_length = client->request.content_len;
	request_info->auth_user = request_info->auth_password = request_info->auth_digest = NULL;
	if (SUCCESS == zend_hash_find(&client->request.headers, "Content-Type", sizeof("Content-Type"), (void**)&val)) {
		request_info->content_type = *val;
	}
} /* }}} */