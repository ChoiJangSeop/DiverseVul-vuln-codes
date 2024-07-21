PHP_METHOD(SoapServer, getFunctions)
{
	soapServicePtr  service;
	HashTable      *ft = NULL;

	SOAP_SERVER_BEGIN_CODE();

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	
	FETCH_THIS_SERVICE(service);

	array_init(return_value);
	if (service->type == SOAP_OBJECT) {
		ft = &(Z_OBJCE_P(service->soap_object)->function_table);
	} else if (service->type == SOAP_CLASS) {
		ft = &service->soap_class.ce->function_table;
	} else if (service->soap_functions.functions_all == TRUE) {
		ft = EG(function_table);
	} else if (service->soap_functions.ft != NULL) {
		zval **name;
		HashPosition pos;

		zend_hash_internal_pointer_reset_ex(service->soap_functions.ft, &pos);
		while (zend_hash_get_current_data_ex(service->soap_functions.ft, (void **)&name, &pos) != FAILURE) {
			add_next_index_string(return_value, Z_STRVAL_PP(name), 1);
			zend_hash_move_forward_ex(service->soap_functions.ft, &pos);
		}
	}
	if (ft != NULL) {
		zend_function *f;
		HashPosition pos;
		zend_hash_internal_pointer_reset_ex(ft, &pos);
		while (zend_hash_get_current_data_ex(ft, (void **)&f, &pos) != FAILURE) {
			if ((service->type != SOAP_OBJECT && service->type != SOAP_CLASS) || (f->common.fn_flags & ZEND_ACC_PUBLIC)) {
				add_next_index_string(return_value, f->common.function_name, 1);
			}
			zend_hash_move_forward_ex(ft, &pos);
		}
	}

	SOAP_SERVER_END_CODE();
}