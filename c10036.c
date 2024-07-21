static sdlPtr load_wsdl(zval *this_ptr, char *struri)
{
	sdlCtx ctx;
	int i,n;

	memset(&ctx,0,sizeof(ctx));
	ctx.sdl = emalloc(sizeof(sdl));
	memset(ctx.sdl, 0, sizeof(sdl));
	ctx.sdl->source = estrdup(struri);
	zend_hash_init(&ctx.sdl->functions, 0, NULL, delete_function, 0);

	zend_hash_init(&ctx.docs, 0, NULL, delete_document, 0);
	zend_hash_init(&ctx.messages, 0, NULL, NULL, 0);
	zend_hash_init(&ctx.bindings, 0, NULL, NULL, 0);
	zend_hash_init(&ctx.portTypes, 0, NULL, NULL, 0);
	zend_hash_init(&ctx.services,  0, NULL, NULL, 0);

	load_wsdl_ex(this_ptr, struri,&ctx, 0);
	schema_pass2(&ctx);

	n = zend_hash_num_elements(&ctx.services);
	if (n > 0) {
		zend_hash_internal_pointer_reset(&ctx.services);
		for (i = 0; i < n; i++) {
			xmlNodePtr service, tmp;
			xmlNodePtr trav, port;
			int has_soap_port = 0;

			service = tmp = zend_hash_get_current_data_ptr(&ctx.services);

			trav = service->children;
			while (trav != NULL) {
				xmlAttrPtr type, name, bindingAttr, location;
				xmlNodePtr portType, operation;
				xmlNodePtr address, binding, trav2;
				char *ctype;
				sdlBindingPtr tmpbinding;
				char *wsdl_soap_namespace = NULL;

				if (!is_wsdl_element(trav) || node_is_equal(trav,"documentation")) {
					trav = trav->next;
					continue;
				}
				if (!node_is_equal(trav,"port")) {
					soap_error1(E_ERROR, "Parsing WSDL: Unexpected WSDL element <%s>", trav->name);
				}

				port = trav;

				tmpbinding = emalloc(sizeof(sdlBinding));
				memset(tmpbinding, 0, sizeof(sdlBinding));

				bindingAttr = get_attribute(port->properties, "binding");
				if (bindingAttr == NULL) {
					soap_error0(E_ERROR, "Parsing WSDL: No binding associated with <port>");
				}

				/* find address and figure out binding type */
				address = NULL;
				trav2 = port->children;
				while (trav2 != NULL) {
					if (node_is_equal(trav2,"address") && trav2->ns) {
						if (!strncmp((char*)trav2->ns->href, WSDL_SOAP11_NAMESPACE, sizeof(WSDL_SOAP11_NAMESPACE))) {
							address = trav2;
							wsdl_soap_namespace = WSDL_SOAP11_NAMESPACE;
							tmpbinding->bindingType = BINDING_SOAP;
						} else if (!strncmp((char*)trav2->ns->href, WSDL_SOAP12_NAMESPACE, sizeof(WSDL_SOAP12_NAMESPACE))) {
							address = trav2;
							wsdl_soap_namespace = WSDL_SOAP12_NAMESPACE;
							tmpbinding->bindingType = BINDING_SOAP;
						} else if (!strncmp((char*)trav2->ns->href, RPC_SOAP12_NAMESPACE, sizeof(RPC_SOAP12_NAMESPACE))) {
							address = trav2;
							wsdl_soap_namespace = RPC_SOAP12_NAMESPACE;
							tmpbinding->bindingType = BINDING_SOAP;
						} else if (!strncmp((char*)trav2->ns->href, WSDL_HTTP11_NAMESPACE, sizeof(WSDL_HTTP11_NAMESPACE))) {
							address = trav2;
							tmpbinding->bindingType = BINDING_HTTP;
						} else if (!strncmp((char*)trav2->ns->href, WSDL_HTTP12_NAMESPACE, sizeof(WSDL_HTTP12_NAMESPACE))) {
							address = trav2;
							tmpbinding->bindingType = BINDING_HTTP;
						}
					}
					if (trav2 != address && is_wsdl_element(trav2) && !node_is_equal(trav2,"documentation")) {
						soap_error1(E_ERROR, "Parsing WSDL: Unexpected WSDL element <%s>", trav2->name);
					}
				  trav2 = trav2->next;
				}
				if (!address || tmpbinding->bindingType == BINDING_HTTP) {
					if (has_soap_port || trav->next || i < n-1) {
						efree(tmpbinding);
						trav = trav->next;
						continue;
					} else if (!address) {
						soap_error0(E_ERROR, "Parsing WSDL: No address associated with <port>");
					}
				}
				has_soap_port = 1;

				location = get_attribute(address->properties, "location");
				if (!location) {
					soap_error0(E_ERROR, "Parsing WSDL: No location associated with <port>");
				}

				tmpbinding->location = estrdup((char*)location->children->content);

				ctype = strrchr((char*)bindingAttr->children->content,':');
				if (ctype == NULL) {
					ctype = (char*)bindingAttr->children->content;
				} else {
					++ctype;
				}
				if ((tmp = zend_hash_str_find_ptr(&ctx.bindings, ctype, strlen(ctype))) == NULL) {
					soap_error1(E_ERROR, "Parsing WSDL: No <binding> element with name '%s'", ctype);
				}
				binding = tmp;

				if (tmpbinding->bindingType == BINDING_SOAP) {
					sdlSoapBindingPtr soapBinding;
					xmlNodePtr soapBindingNode;
					xmlAttrPtr tmp;

					soapBinding = emalloc(sizeof(sdlSoapBinding));
					memset(soapBinding, 0, sizeof(sdlSoapBinding));
					soapBinding->style = SOAP_DOCUMENT;

					soapBindingNode = get_node_ex(binding->children, "binding", wsdl_soap_namespace);
					if (soapBindingNode) {
						tmp = get_attribute(soapBindingNode->properties, "style");
						if (tmp && !strncmp((char*)tmp->children->content, "rpc", sizeof("rpc"))) {
							soapBinding->style = SOAP_RPC;
						}

						tmp = get_attribute(soapBindingNode->properties, "transport");
						if (tmp) {
							if (strncmp((char*)tmp->children->content, WSDL_HTTP_TRANSPORT, sizeof(WSDL_HTTP_TRANSPORT)) == 0) {
								soapBinding->transport = SOAP_TRANSPORT_HTTP;
							} else {
								/* try the next binding */
								efree(soapBinding);
								efree(tmpbinding->location);
								efree(tmpbinding);
								trav = trav->next;
								continue;
							}
						}
					}
					tmpbinding->bindingAttributes = (void *)soapBinding;
				}

				name = get_attribute(binding->properties, "name");
				if (name == NULL) {
					soap_error0(E_ERROR, "Parsing WSDL: Missing 'name' attribute for <binding>");
				}
				tmpbinding->name = estrdup((char*)name->children->content);

				type = get_attribute(binding->properties, "type");
				if (type == NULL) {
					soap_error0(E_ERROR, "Parsing WSDL: Missing 'type' attribute for <binding>");
				}

				ctype = strrchr((char*)type->children->content,':');
				if (ctype == NULL) {
					ctype = (char*)type->children->content;
				} else {
					++ctype;
				}
				if ((tmp = zend_hash_str_find_ptr(&ctx.portTypes, ctype, strlen(ctype))) == NULL) {
					soap_error1(E_ERROR, "Parsing WSDL: Missing <portType> with name '%s'", name->children->content);
				}
				portType = tmp;

				trav2 = binding->children;
				while (trav2 != NULL) {
					sdlFunctionPtr function;
					xmlNodePtr input, output, fault, portTypeOperation, trav3;
					xmlAttrPtr op_name, paramOrder;

					if ((tmpbinding->bindingType == BINDING_SOAP &&
					    node_is_equal_ex(trav2, "binding", wsdl_soap_namespace)) ||
					    !is_wsdl_element(trav2) ||
					    node_is_equal(trav2,"documentation")) {
						trav2 = trav2->next;
						continue;
					}
					if (!node_is_equal(trav2,"operation")) {
						soap_error1(E_ERROR, "Parsing WSDL: Unexpected WSDL element <%s>", trav2->name);
					}

					operation = trav2;

					op_name = get_attribute(operation->properties, "name");
					if (op_name == NULL) {
						soap_error0(E_ERROR, "Parsing WSDL: Missing 'name' attribute for <operation>");
					}

					trav3 = operation->children;
					while  (trav3 != NULL) {
						if (tmpbinding->bindingType == BINDING_SOAP &&
						    node_is_equal_ex(trav3, "operation", wsdl_soap_namespace)) {
						} else if (is_wsdl_element(trav3) &&
						           !node_is_equal(trav3,"input") &&
						           !node_is_equal(trav3,"output") &&
						           !node_is_equal(trav3,"fault") &&
						           !node_is_equal(trav3,"documentation")) {
							soap_error1(E_ERROR, "Parsing WSDL: Unexpected WSDL element <%s>", trav3->name);
						}
						trav3 = trav3->next;
					}

					portTypeOperation = get_node_with_attribute_ex(portType->children, "operation", WSDL_NAMESPACE, "name", (char*)op_name->children->content, NULL);
					if (portTypeOperation == NULL) {
						soap_error1(E_ERROR, "Parsing WSDL: Missing <portType>/<operation> with name '%s'", op_name->children->content);
					}

					function = emalloc(sizeof(sdlFunction));
					memset(function, 0, sizeof(sdlFunction));
					function->functionName = estrdup((char*)op_name->children->content);

					if (tmpbinding->bindingType == BINDING_SOAP) {
						sdlSoapBindingFunctionPtr soapFunctionBinding;
						sdlSoapBindingPtr soapBinding;
						xmlNodePtr soapOperation;
						xmlAttrPtr tmp;

						soapFunctionBinding = emalloc(sizeof(sdlSoapBindingFunction));
						memset(soapFunctionBinding, 0, sizeof(sdlSoapBindingFunction));
						soapBinding = (sdlSoapBindingPtr)tmpbinding->bindingAttributes;
						soapFunctionBinding->style = soapBinding->style;

						soapOperation = get_node_ex(operation->children, "operation", wsdl_soap_namespace);
						if (soapOperation) {
							tmp = get_attribute(soapOperation->properties, "soapAction");
							if (tmp) {
								soapFunctionBinding->soapAction = estrdup((char*)tmp->children->content);
							}

							tmp = get_attribute(soapOperation->properties, "style");
							if (tmp) {
								if (!strncmp((char*)tmp->children->content, "rpc", sizeof("rpc"))) {
									soapFunctionBinding->style = SOAP_RPC;
								} else {
									soapFunctionBinding->style = SOAP_DOCUMENT;
								}
							} else {
								soapFunctionBinding->style = soapBinding->style;
							}
						}

						function->bindingAttributes = (void *)soapFunctionBinding;
					}

					input = get_node_ex(portTypeOperation->children, "input", WSDL_NAMESPACE);
					if (input != NULL) {
						xmlAttrPtr message;

						message = get_attribute(input->properties, "message");
						if (message == NULL) {
							soap_error1(E_ERROR, "Parsing WSDL: Missing name for <input> of '%s'", op_name->children->content);
						}
						function->requestParameters = wsdl_message(&ctx, message->children->content);

/* FIXME
						xmlAttrPtr name = get_attribute(input->properties, "name");
						if (name != NULL) {
							function->requestName = estrdup(name->children->content);
						} else {
*/
						{
							function->requestName = estrdup(function->functionName);
						}

						if (tmpbinding->bindingType == BINDING_SOAP) {
							input = get_node_ex(operation->children, "input", WSDL_NAMESPACE);
							if (input != NULL) {
								sdlSoapBindingFunctionPtr soapFunctionBinding = function->bindingAttributes;
								wsdl_soap_binding_body(&ctx, input, wsdl_soap_namespace, &soapFunctionBinding->input, function->requestParameters);
							}
						}
					}

					output = get_node_ex(portTypeOperation->children, "output", WSDL_NAMESPACE);
					if (output != NULL) {
						xmlAttrPtr message;

						message = get_attribute(output->properties, "message");
						if (message == NULL) {
							soap_error1(E_ERROR, "Parsing WSDL: Missing name for <output> of '%s'", op_name->children->content);
						}
						function->responseParameters = wsdl_message(&ctx, message->children->content);

/* FIXME
						xmlAttrPtr name = get_attribute(output->properties, "name");
						if (name != NULL) {
							function->responseName = estrdup(name->children->content);
						} else if (input == NULL) {
							function->responseName = estrdup(function->functionName);
						} else {
*/
						{
							int len = strlen(function->functionName);
							function->responseName = emalloc(len + sizeof("Response"));
							memcpy(function->responseName, function->functionName, len);
							memcpy(function->responseName+len, "Response", sizeof("Response"));
						}

						if (tmpbinding->bindingType == BINDING_SOAP) {
							output = get_node_ex(operation->children, "output", WSDL_NAMESPACE);
							if (output != NULL) {
								sdlSoapBindingFunctionPtr soapFunctionBinding = function->bindingAttributes;
								wsdl_soap_binding_body(&ctx, output, wsdl_soap_namespace, &soapFunctionBinding->output, function->responseParameters);
							}
						}
					}

					paramOrder = get_attribute(portTypeOperation->properties, "parameterOrder");
					if (paramOrder) {
						/* FIXME: */
					}

					fault = portTypeOperation->children;
					while (fault != NULL) {
						if (node_is_equal_ex(fault, "fault", WSDL_NAMESPACE)) {
							xmlAttrPtr message, name;
							sdlFaultPtr f;

							name = get_attribute(fault->properties, "name");
							if (name == NULL) {
								soap_error1(E_ERROR, "Parsing WSDL: Missing name for <fault> of '%s'", op_name->children->content);
							}
							message = get_attribute(fault->properties, "message");
							if (message == NULL) {
								soap_error1(E_ERROR, "Parsing WSDL: Missing name for <output> of '%s'", op_name->children->content);
							}

							f = emalloc(sizeof(sdlFault));
							memset(f, 0, sizeof(sdlFault));

							f->name = estrdup((char*)name->children->content);
							f->details = wsdl_message(&ctx, message->children->content);
							if (f->details == NULL || zend_hash_num_elements(f->details) > 1) {
								soap_error1(E_ERROR, "Parsing WSDL: The fault message '%s' must have a single part", message->children->content);
							}

							if (tmpbinding->bindingType == BINDING_SOAP) {
								xmlNodePtr soap_fault = get_node_with_attribute_ex(operation->children, "fault", WSDL_NAMESPACE, "name", f->name, NULL);
								if (soap_fault != NULL) {
									xmlNodePtr trav = soap_fault->children;
									while (trav != NULL) {
										if (node_is_equal_ex(trav, "fault", wsdl_soap_namespace)) {
											xmlAttrPtr tmp;
										  sdlSoapBindingFunctionFaultPtr binding;

											binding = f->bindingAttributes = emalloc(sizeof(sdlSoapBindingFunctionFault));
											memset(f->bindingAttributes, 0, sizeof(sdlSoapBindingFunctionFault));

											tmp = get_attribute(trav->properties, "use");
											if (tmp && !strncmp((char*)tmp->children->content, "encoded", sizeof("encoded"))) {
												binding->use = SOAP_ENCODED;
											} else {
												binding->use = SOAP_LITERAL;
											}

											tmp = get_attribute(trav->properties, "namespace");
											if (tmp) {
												binding->ns = estrdup((char*)tmp->children->content);
											}

											if (binding->use == SOAP_ENCODED) {
												tmp = get_attribute(trav->properties, "encodingStyle");
												if (tmp) {
													if (strncmp((char*)tmp->children->content, SOAP_1_1_ENC_NAMESPACE, sizeof(SOAP_1_1_ENC_NAMESPACE)) == 0) {
														binding->encodingStyle = SOAP_ENCODING_1_1;
													} else if (strncmp((char*)tmp->children->content, SOAP_1_2_ENC_NAMESPACE, sizeof(SOAP_1_2_ENC_NAMESPACE)) == 0) {
														binding->encodingStyle = SOAP_ENCODING_1_2;
													} else {
														soap_error1(E_ERROR, "Parsing WSDL: Unknown encodingStyle '%s'", tmp->children->content);
													}
												} else {
													soap_error0(E_ERROR, "Parsing WSDL: Unspecified encodingStyle");
												}
											}
										} else if (is_wsdl_element(trav) && !node_is_equal(trav,"documentation")) {
											soap_error1(E_ERROR, "Parsing WSDL: Unexpected WSDL element <%s>", trav->name);
										}
										trav = trav->next;
									}
								}
							}
							if (function->faults == NULL) {
								function->faults = emalloc(sizeof(HashTable));
								zend_hash_init(function->faults, 0, NULL, delete_fault, 0);
							}
							if (zend_hash_str_add_ptr(function->faults, f->name, strlen(f->name), f) == NULL) {
								soap_error2(E_ERROR, "Parsing WSDL: <fault> with name '%s' already defined in '%s'", f->name, op_name->children->content);
							}
						}
						fault = fault->next;
					}

					function->binding = tmpbinding;

					{
						char *tmp = estrdup(function->functionName);
						int  len = strlen(tmp);

						if (zend_hash_str_add_ptr(&ctx.sdl->functions, php_strtolower(tmp, len), len, function) == NULL) {
							zend_hash_next_index_insert_ptr(&ctx.sdl->functions, function);
						}
						efree(tmp);
						if (function->requestName != NULL && strcmp(function->requestName,function->functionName) != 0) {
							if (ctx.sdl->requests == NULL) {
								ctx.sdl->requests = emalloc(sizeof(HashTable));
								zend_hash_init(ctx.sdl->requests, 0, NULL, NULL, 0);
							}
							tmp = estrdup(function->requestName);
							len = strlen(tmp);
							zend_hash_str_add_ptr(ctx.sdl->requests, php_strtolower(tmp, len), len, function);
							efree(tmp);
						}
					}
					trav2 = trav2->next;
				}

				if (!ctx.sdl->bindings) {
					ctx.sdl->bindings = emalloc(sizeof(HashTable));
					zend_hash_init(ctx.sdl->bindings, 0, NULL, delete_binding, 0);
				}

				if (!zend_hash_str_add_ptr(ctx.sdl->bindings, tmpbinding->name, strlen(tmpbinding->name), tmpbinding)) {
					zend_hash_next_index_insert_ptr(ctx.sdl->bindings, tmpbinding);
				}
				trav= trav->next;
			}

			zend_hash_move_forward(&ctx.services);
		}
	} else {
		soap_error0(E_ERROR, "Parsing WSDL: Couldn't bind to service");
	}

	if (ctx.sdl->bindings == NULL || ctx.sdl->bindings->nNumOfElements == 0) {
		soap_error0(E_ERROR, "Parsing WSDL: Could not find any usable binding services in WSDL.");
	}

	zend_hash_destroy(&ctx.messages);
	zend_hash_destroy(&ctx.bindings);
	zend_hash_destroy(&ctx.portTypes);
	zend_hash_destroy(&ctx.services);
	zend_hash_destroy(&ctx.docs);

	return ctx.sdl;
}