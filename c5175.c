ZEND_VM_HANDLER(8, ZEND_CONCAT, CONST|TMPVAR|CV, CONST|TMPVAR|CV, SPEC(NO_CONST_CONST))
{
	USE_OPLINE
	zend_free_op free_op1, free_op2;
	zval *op1, *op2;

	op1 = GET_OP1_ZVAL_PTR_UNDEF(BP_VAR_R);
	op2 = GET_OP2_ZVAL_PTR_UNDEF(BP_VAR_R);

	if ((OP1_TYPE == IS_CONST || EXPECTED(Z_TYPE_P(op1) == IS_STRING)) &&
	    (OP2_TYPE == IS_CONST || EXPECTED(Z_TYPE_P(op2) == IS_STRING))) {
		zend_string *op1_str = Z_STR_P(op1);
		zend_string *op2_str = Z_STR_P(op2);
		zend_string *str;

		if (OP1_TYPE != IS_CONST && UNEXPECTED(ZSTR_LEN(op1_str) == 0)) {
			if (OP2_TYPE == IS_CONST || OP2_TYPE == IS_CV) {
				ZVAL_STR_COPY(EX_VAR(opline->result.var), op2_str);
			} else {
				ZVAL_STR(EX_VAR(opline->result.var), op2_str);
			}
			if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
				zend_string_release_ex(op1_str, 0);
			}
		} else if (OP2_TYPE != IS_CONST && UNEXPECTED(ZSTR_LEN(op2_str) == 0)) {
			if (OP1_TYPE == IS_CONST || OP1_TYPE == IS_CV) {
				ZVAL_STR_COPY(EX_VAR(opline->result.var), op1_str);
			} else {
				ZVAL_STR(EX_VAR(opline->result.var), op1_str);
			}
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				zend_string_release_ex(op2_str, 0);
			}
		} else if (OP1_TYPE != IS_CONST && OP1_TYPE != IS_CV &&
		    !ZSTR_IS_INTERNED(op1_str) && GC_REFCOUNT(op1_str) == 1) {
		    size_t len = ZSTR_LEN(op1_str);

			str = zend_string_extend(op1_str, len + ZSTR_LEN(op2_str), 0);
			memcpy(ZSTR_VAL(str) + len, ZSTR_VAL(op2_str), ZSTR_LEN(op2_str)+1);
			ZVAL_NEW_STR(EX_VAR(opline->result.var), str);
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				zend_string_release_ex(op2_str, 0);
			}
		} else {
			str = zend_string_alloc(ZSTR_LEN(op1_str) + ZSTR_LEN(op2_str), 0);
			memcpy(ZSTR_VAL(str), ZSTR_VAL(op1_str), ZSTR_LEN(op1_str));
			memcpy(ZSTR_VAL(str) + ZSTR_LEN(op1_str), ZSTR_VAL(op2_str), ZSTR_LEN(op2_str)+1);
			ZVAL_NEW_STR(EX_VAR(opline->result.var), str);
			if (OP1_TYPE & (IS_TMP_VAR|IS_VAR)) {
				zend_string_release_ex(op1_str, 0);
			}
			if (OP2_TYPE & (IS_TMP_VAR|IS_VAR)) {
				zend_string_release_ex(op2_str, 0);
			}
		}
		ZEND_VM_NEXT_OPCODE();
	} else {
		SAVE_OPLINE();

		if (OP1_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op1) == IS_UNDEF)) {
			op1 = ZVAL_UNDEFINED_OP1();
		}
		if (OP2_TYPE == IS_CV && UNEXPECTED(Z_TYPE_P(op2) == IS_UNDEF)) {
			op2 = ZVAL_UNDEFINED_OP2();
		}
		concat_function(EX_VAR(opline->result.var), op1, op2);
		FREE_OP1();
		FREE_OP2();
		ZEND_VM_NEXT_OPCODE_CHECK_EXCEPTION();
	}
}