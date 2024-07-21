PHP_FUNCTION(pcntl_exec)
{
	zval *args = NULL, *envs = NULL;
	zval **element;
	HashTable *args_hash, *envs_hash;
	int argc = 0, argi = 0;
	int envc = 0, envi = 0;
	int return_val = 0;
	char **argv = NULL, **envp = NULL;
	char **current_arg, **pair;
	int pair_length;
	char *key;
	uint key_length;
	char *path;
	int path_len;
	ulong key_num;
		
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|aa", &path, &path_len, &args, &envs) == FAILURE) {
		return;
	}
	
	if (ZEND_NUM_ARGS() > 1) {
		/* Build argument list */
		args_hash = HASH_OF(args);
		argc = zend_hash_num_elements(args_hash);
		
		argv = safe_emalloc((argc + 2), sizeof(char *), 0);
		*argv = path;
		for ( zend_hash_internal_pointer_reset(args_hash), current_arg = argv+1; 
			(argi < argc && (zend_hash_get_current_data(args_hash, (void **) &element) == SUCCESS));
			(argi++, current_arg++, zend_hash_move_forward(args_hash)) ) {

			convert_to_string_ex(element);
			*current_arg = Z_STRVAL_PP(element);
		}
		*(current_arg) = NULL;
	} else {
		argv = emalloc(2 * sizeof(char *));
		*argv = path;
		*(argv+1) = NULL;
	}

	if ( ZEND_NUM_ARGS() == 3 ) {
		/* Build environment pair list */
		envs_hash = HASH_OF(envs);
		envc = zend_hash_num_elements(envs_hash);
		
		envp = safe_emalloc((envc + 1), sizeof(char *), 0);
		for ( zend_hash_internal_pointer_reset(envs_hash), pair = envp; 
			(envi < envc && (zend_hash_get_current_data(envs_hash, (void **) &element) == SUCCESS));
			(envi++, pair++, zend_hash_move_forward(envs_hash)) ) {
			switch (return_val = zend_hash_get_current_key_ex(envs_hash, &key, &key_length, &key_num, 0, NULL)) {
				case HASH_KEY_IS_LONG:
					key = emalloc(101);
					snprintf(key, 100, "%ld", key_num);
					key_length = strlen(key);
					break;
				case HASH_KEY_NON_EXISTANT:
					pair--;
					continue;
			}

			convert_to_string_ex(element);

			/* Length of element + equal sign + length of key + null */ 
			pair_length = Z_STRLEN_PP(element) + key_length + 2;
			*pair = emalloc(pair_length);
			strlcpy(*pair, key, key_length); 
			strlcat(*pair, "=", pair_length);
			strlcat(*pair, Z_STRVAL_PP(element), pair_length);
			
			/* Cleanup */
			if (return_val == HASH_KEY_IS_LONG) efree(key);
		}
		*(pair) = NULL;

		if (execve(path, argv, envp) == -1) {
			PCNTL_G(last_error) = errno;
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error has occurred: (errno %d) %s", errno, strerror(errno));
		}
	
		/* Cleanup */
		for (pair = envp; *pair != NULL; pair++) efree(*pair);
		efree(envp);
	} else {

		if (execv(path, argv) == -1) {
			PCNTL_G(last_error) = errno;
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error has occurred: (errno %d) %s", errno, strerror(errno));
		}
	}

	efree(argv);
	
	RETURN_FALSE;
}