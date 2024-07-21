static void load_wsdl_ex(zval *this_ptr, char *struri, sdlCtx *ctx, int include)
{
	sdlPtr tmpsdl = ctx->sdl;
	xmlDocPtr wsdl;
	xmlNodePtr root, definitions, trav;
	xmlAttrPtr targetNamespace;

	if (zend_hash_str_exists(&ctx->docs, struri, strlen(struri))) {
		return;
	}

	sdl_set_uri_credentials(ctx, struri);
	wsdl = soap_xmlParseFile(struri);
	sdl_restore_uri_credentials(ctx);

	if (!wsdl) {
		xmlErrorPtr xmlErrorPtr = xmlGetLastError();

		if (xmlErrorPtr) {
			soap_error2(E_ERROR, "Parsing WSDL: Couldn't load from '%s' : %s", struri, xmlErrorPtr->message);
		} else {
			soap_error1(E_ERROR, "Parsing WSDL: Couldn't load from '%s'", struri);
		}
	}

	zend_hash_str_add_ptr(&ctx->docs, struri, strlen(struri), wsdl);

	root = wsdl->children;
	definitions = get_node_ex(root, "definitions", WSDL_NAMESPACE);
	if (!definitions) {
		if (include) {
			xmlNodePtr schema = get_node_ex(root, "schema", XSD_NAMESPACE);
			if (schema) {
				load_schema(ctx, schema);
				return;
			}
		}
		soap_error1(E_ERROR, "Parsing WSDL: Couldn't find <definitions> in '%s'", struri);
	}

	if (!include) {
		targetNamespace = get_attribute(definitions->properties, "targetNamespace");
		if (targetNamespace) {
			tmpsdl->target_ns = estrdup((char*)targetNamespace->children->content);
		}
	}

	trav = definitions->children;
	while (trav != NULL) {
		if (!is_wsdl_element(trav)) {
			trav = trav->next;
			continue;
		}
		if (node_is_equal(trav,"types")) {
			/* TODO: Only one "types" is allowed */
			xmlNodePtr trav2 = trav->children;

			while (trav2 != NULL) {
				if (node_is_equal_ex(trav2, "schema", XSD_NAMESPACE)) {
					load_schema(ctx, trav2);
				} else if (is_wsdl_element(trav2) && !node_is_equal(trav2,"documentation")) {
					soap_error1(E_ERROR, "Parsing WSDL: Unexpected WSDL element <%s>", trav2->name);
				}
				trav2 = trav2->next;
			}
		} else if (node_is_equal(trav,"import")) {
			/* TODO: namespace ??? */
			xmlAttrPtr tmp = get_attribute(trav->properties, "location");
			if (tmp) {
				xmlChar *uri;
				xmlChar *base = xmlNodeGetBase(trav->doc, trav);

				if (base == NULL) {
					uri = xmlBuildURI(tmp->children->content, trav->doc->URL);
				} else {
					uri = xmlBuildURI(tmp->children->content, base);
					xmlFree(base);
				}
				load_wsdl_ex(this_ptr, (char*)uri, ctx, 1);
				xmlFree(uri);
			}

		} else if (node_is_equal(trav,"message")) {
			xmlAttrPtr name = get_attribute(trav->properties, "name");
			if (name && name->children && name->children->content) {
				if (zend_hash_str_add_ptr(&ctx->messages, (char*)name->children->content, xmlStrlen(name->children->content), trav) == NULL) {
					soap_error1(E_ERROR, "Parsing WSDL: <message> '%s' already defined", name->children->content);
				}
			} else {
				soap_error0(E_ERROR, "Parsing WSDL: <message> has no name attribute");
			}

		} else if (node_is_equal(trav,"portType")) {
			xmlAttrPtr name = get_attribute(trav->properties, "name");
			if (name && name->children && name->children->content) {
				if (zend_hash_str_add_ptr(&ctx->portTypes, (char*)name->children->content, xmlStrlen(name->children->content), trav) == NULL) {
					soap_error1(E_ERROR, "Parsing WSDL: <portType> '%s' already defined", name->children->content);
				}
			} else {
				soap_error0(E_ERROR, "Parsing WSDL: <portType> has no name attribute");
			}

		} else if (node_is_equal(trav,"binding")) {
			xmlAttrPtr name = get_attribute(trav->properties, "name");
			if (name && name->children && name->children->content) {
				if (zend_hash_str_add_ptr(&ctx->bindings, (char*)name->children->content, xmlStrlen(name->children->content), trav) == NULL) {
					soap_error1(E_ERROR, "Parsing WSDL: <binding> '%s' already defined", name->children->content);
				}
			} else {
				soap_error0(E_ERROR, "Parsing WSDL: <binding> has no name attribute");
			}

		} else if (node_is_equal(trav,"service")) {
			xmlAttrPtr name = get_attribute(trav->properties, "name");
			if (name && name->children && name->children->content) {
				if (zend_hash_str_add_ptr(&ctx->services, (char*)name->children->content, xmlStrlen(name->children->content), trav) == NULL) {
					soap_error1(E_ERROR, "Parsing WSDL: <service> '%s' already defined", name->children->content);
				}
			} else {
				soap_error0(E_ERROR, "Parsing WSDL: <service> has no name attribute");
			}
		} else if (!node_is_equal(trav,"documentation")) {
			soap_error1(E_ERROR, "Parsing WSDL: Unexpected WSDL element <%s>", trav->name);
		}
		trav = trav->next;
	}
}