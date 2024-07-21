ves_icall_System_Runtime_CompilerServices_RuntimeHelpers_InitializeArray (MonoArray *array, MonoClassField *field_handle)
{
	MonoClass *klass = array->obj.vtable->klass;
	guint32 size = mono_array_element_size (klass);
	MonoType *type = mono_type_get_underlying_type (&klass->element_class->byval_arg);
	int align;
	const char *field_data;

	if (MONO_TYPE_IS_REFERENCE (type) ||
			(type->type == MONO_TYPE_VALUETYPE &&
				(!mono_type_get_class (type) ||
				mono_type_get_class (type)->has_references))) {
		MonoException *exc = mono_get_exception_argument("array",
			"Cannot initialize array containing references");
		mono_raise_exception (exc);
	}

	if (!(field_handle->type->attrs & FIELD_ATTRIBUTE_HAS_FIELD_RVA)) {
		MonoException *exc = mono_get_exception_argument("field_handle",
			"Field doesn't have an RVA");
		mono_raise_exception (exc);
	}

	size *= array->max_length;
	field_data = mono_field_get_data (field_handle);

	if (size > mono_type_size (field_handle->type, &align)) {
		MonoException *exc = mono_get_exception_argument("field_handle",
			"Field not large enough to fill array");
		mono_raise_exception (exc);
	}

#if G_BYTE_ORDER != G_LITTLE_ENDIAN
#define SWAP(n) {\
	guint ## n *data = (guint ## n *) mono_array_addr (array, char, 0); \
	guint ## n *src = (guint ## n *) field_data; \
	guint ## n *end = (guint ## n *)((char*)src + size); \
\
	for (; src < end; data++, src++) { \
		*data = read ## n (src); \
	} \
}

	/* printf ("Initialize array with elements of %s type\n", klass->element_class->name); */

	switch (type->type) {
	case MONO_TYPE_CHAR:
	case MONO_TYPE_I2:
	case MONO_TYPE_U2:
		SWAP (16);
		break;
	case MONO_TYPE_I4:
	case MONO_TYPE_U4:
	case MONO_TYPE_R4:
		SWAP (32);
		break;
	case MONO_TYPE_I8:
	case MONO_TYPE_U8:
	case MONO_TYPE_R8:
		SWAP (64);
		break;
	default:
		memcpy (mono_array_addr (array, char, 0), field_data, size);
		break;
	}
#else
	memcpy (mono_array_addr (array, char, 0), field_data, size);
#ifdef ARM_FPU_FPA
	if (klass->element_class->byval_arg.type == MONO_TYPE_R8) {
		gint i;
		double tmp;
		double *data = (double*)mono_array_addr (array, double, 0);

		for (i = 0; i < size; i++, data++) {
			readr8 (data, &tmp);
			*data = tmp;
		}
	}
#endif
#endif
}