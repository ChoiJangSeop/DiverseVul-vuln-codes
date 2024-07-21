request_init(VALUE mod) {
    req_class = rb_define_class_under(mod, "Request", rb_cObject);

    rb_define_method(req_class, "to_s", to_s, 0);
    rb_define_method(req_class, "to_h", to_h, 0);
    rb_define_method(req_class, "environment", to_h, 0);
    rb_define_method(req_class, "env", to_h, 0);
    rb_define_method(req_class, "request_method", method, 0);
    rb_define_method(req_class, "script_name", script_name, 0);
    rb_define_method(req_class, "path_info", path_info, 0);
    rb_define_method(req_class, "query_string", query_string, 0);
    rb_define_method(req_class, "server_name", server_name, 0);
    rb_define_method(req_class, "server_port", server_port, 0);
    rb_define_method(req_class, "rack_version", rack_version, 0);
    rb_define_method(req_class, "rack_url_scheme", rack_url_scheme, 0);
    rb_define_method(req_class, "rack_input", rack_input, 0);
    rb_define_method(req_class, "rack_errors", rack_errors, 0);
    rb_define_method(req_class, "rack_multithread", rack_multithread, 0);
    rb_define_method(req_class, "rack_multiprocess", rack_multiprocess, 0);
    rb_define_method(req_class, "rack_run_once", rack_run_once, 0);
    rb_define_method(req_class, "rack_upgrade?", rack_upgrade, 0);
    rb_define_method(req_class, "headers", headers, 0);
    rb_define_method(req_class, "body", body, 0);
    rb_define_method(req_class, "rack_logger", rack_logger, 0);
    rb_define_method(req_class, "call", call, 0);

    new_id = rb_intern("new");

    rack_version_val_val = rb_ary_new();
    rb_ary_push(rack_version_val_val, INT2NUM(1));
    rb_ary_push(rack_version_val_val, INT2NUM(3));
    rb_gc_register_address(&rack_version_val_val);

    stringio_class = rb_const_get(rb_cObject, rb_intern("StringIO"));

    connect_val = rb_str_new_cstr("CONNECT");			rb_gc_register_address(&connect_val);
    content_length_val = rb_str_new_cstr("CONTENT_LENGTH");	rb_gc_register_address(&content_length_val);
    content_type_val = rb_str_new_cstr("CONTENT_TYPE");		rb_gc_register_address(&content_type_val);
    delete_val = rb_str_new_cstr("DELETE");			rb_gc_register_address(&delete_val);
    early_hints_val = rb_str_new_cstr("early_hints");		rb_gc_register_address(&early_hints_val);
    empty_val = rb_str_new_cstr("");				rb_gc_register_address(&empty_val);
    get_val = rb_str_new_cstr("GET");				rb_gc_register_address(&get_val);
    head_val = rb_str_new_cstr("HEAD");				rb_gc_register_address(&head_val);
    http_val = rb_str_new_cstr("http");				rb_gc_register_address(&http_val);
    https_val = rb_str_new_cstr("https");			rb_gc_register_address(&https_val);
    options_val = rb_str_new_cstr("OPTIONS");			rb_gc_register_address(&options_val);
    patch_val = rb_str_new_cstr("PATCH");			rb_gc_register_address(&patch_val);
    path_info_val = rb_str_new_cstr("PATH_INFO");		rb_gc_register_address(&path_info_val);
    post_val = rb_str_new_cstr("POST");				rb_gc_register_address(&post_val);
    put_val = rb_str_new_cstr("PUT");				rb_gc_register_address(&put_val);
    query_string_val = rb_str_new_cstr("QUERY_STRING");		rb_gc_register_address(&query_string_val);
    rack_errors_val = rb_str_new_cstr("rack.errors");		rb_gc_register_address(&rack_errors_val);
    rack_hijack_io_val = rb_str_new_cstr("rack.hijack_io");	rb_gc_register_address(&rack_hijack_io_val);
    rack_hijack_val = rb_str_new_cstr("rack.hijack");		rb_gc_register_address(&rack_hijack_val);
    rack_hijackq_val = rb_str_new_cstr("rack.hijack?");		rb_gc_register_address(&rack_hijackq_val);
    rack_input_val = rb_str_new_cstr("rack.input");		rb_gc_register_address(&rack_input_val);
    rack_logger_val = rb_str_new_cstr("rack.logger");		rb_gc_register_address(&rack_logger_val);
    rack_multiprocess_val = rb_str_new_cstr("rack.multiprocess");rb_gc_register_address(&rack_multiprocess_val);
    rack_multithread_val = rb_str_new_cstr("rack.multithread");	rb_gc_register_address(&rack_multithread_val);
    rack_run_once_val = rb_str_new_cstr("rack.run_once");	rb_gc_register_address(&rack_run_once_val);
    rack_upgrade_val = rb_str_new_cstr("rack.upgrade?");	rb_gc_register_address(&rack_upgrade_val);
    rack_url_scheme_val = rb_str_new_cstr("rack.url_scheme");	rb_gc_register_address(&rack_url_scheme_val);
    rack_version_val = rb_str_new_cstr("rack.version");		rb_gc_register_address(&rack_version_val);
    request_method_val = rb_str_new_cstr("REQUEST_METHOD");	rb_gc_register_address(&request_method_val);
    script_name_val = rb_str_new_cstr("SCRIPT_NAME");		rb_gc_register_address(&script_name_val);
    server_name_val = rb_str_new_cstr("SERVER_NAME");		rb_gc_register_address(&server_name_val);
    server_port_val = rb_str_new_cstr("SERVER_PORT");		rb_gc_register_address(&server_port_val);
    slash_val = rb_str_new_cstr("/");				rb_gc_register_address(&slash_val);

    sse_sym = ID2SYM(rb_intern("sse"));				rb_gc_register_address(&sse_sym);
    websocket_sym = ID2SYM(rb_intern("websocket"));		rb_gc_register_address(&websocket_sym);
}