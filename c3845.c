static int add_array_entry(const char* loc_name, zval* hash_arr, char* key_name TSRMLS_DC)
{
	char*   key_value 	= NULL;
	char*   cur_key_name	= NULL;
	char*   token        	= NULL;
	char*   last_ptr  	= NULL;

	int	result		= 0;
	int 	cur_result  	= 0;
	int 	cnt  		= 0;


	if( strcmp(key_name , LOC_PRIVATE_TAG)==0 ){
		key_value = get_private_subtags( loc_name );
		result = 1;
	} else {
		key_value = get_icu_value_internal( loc_name , key_name , &result,1 );
	}
	if( (strcmp(key_name , LOC_PRIVATE_TAG)==0) || 
		( strcmp(key_name , LOC_VARIANT_TAG)==0) ){
		if( result > 0 && key_value){
			/* Tokenize on the "_" or "-"  */
			token = php_strtok_r( key_value , DELIMITER ,&last_ptr);	
			if( cur_key_name ){
				efree( cur_key_name);
			}
			cur_key_name = (char*)ecalloc( 25,  25);
			sprintf( cur_key_name , "%s%d", key_name , cnt++);	
			add_assoc_string( hash_arr, cur_key_name , token ,TRUE );
			/* tokenize on the "_" or "-" and stop  at singleton if any */
			while( (token = php_strtok_r(NULL , DELIMITER , &last_ptr)) && (strlen(token)>1) ){
				sprintf( cur_key_name , "%s%d", key_name , cnt++);	
				add_assoc_string( hash_arr, cur_key_name , token , TRUE );
			}
/*
			if( strcmp(key_name, LOC_PRIVATE_TAG) == 0 ){
			}
*/
		}
	} else {
		if( result == 1 ){
			add_assoc_string( hash_arr, key_name , key_value , TRUE );
			cur_result = 1;
		}
	}

	if( cur_key_name ){
		efree( cur_key_name);
	}
	/*if( key_name != LOC_PRIVATE_TAG && key_value){*/
	if( key_value){
		efree(key_value);	
	}
	return cur_result;
}