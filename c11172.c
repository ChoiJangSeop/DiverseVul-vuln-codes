R_API RBinJavaStackMapFrame *r_bin_java_stack_map_frame_new(ut8 *buffer, ut64 sz, RBinJavaStackMapFrame *p_frame, ut64 buf_offset) {
	RBinJavaStackMapFrame *stack_frame = r_bin_java_default_stack_frame ();
	RBinJavaVerificationObj *se = NULL;
	ut64 offset = 0;
	if (!stack_frame) {
		return NULL;
	}
	stack_frame->tag = buffer[offset];
	offset += 1;
	stack_frame->metas->type_info = (void *) r_bin_java_determine_stack_frame_type (stack_frame->tag);
	stack_frame->type = ((RBinJavaStackMapFrameMetas *) stack_frame->metas->type_info)->type;
	stack_frame->file_offset = buf_offset;
	stack_frame->p_stack_frame = p_frame;
	switch (stack_frame->type) {
	case R_BIN_JAVA_STACK_FRAME_SAME:
		// Maybe?  1. Copy the previous frames locals and set the locals count.
		// copy_type_info_to_stack_frame_list_up_to_idx (p_frame->local_items, stack_frame->local_items, idx);
		if (p_frame) {
			stack_frame->number_of_locals = p_frame->number_of_locals;
		} else {
			IFINT eprintf ("><?><\n");
			IFDBG eprintf ("Unable to set previous stackframe with the number of locals (current info.code_attr.implicit_frame was probably not set :/)");
		}
		IFDBG eprintf ("r_bin_java_stack_map_frame_new: TODO Stack Frame Same Locals Condition is untested, so there may be issues.\n");
		break;
	case R_BIN_JAVA_STACK_FRAME_SAME_LOCALS_1:
		// 1. Read the stack type
		stack_frame->number_of_stack_items = 1;
		se = r_bin_java_read_from_buffer_verification_info_new (buffer + offset, sz - offset, buf_offset + offset);
		IFDBG eprintf ("r_bin_java_stack_map_frame_new: Parsed R_BIN_JAVA_STACK_FRAME_SAME_LOCALS_1.\n");
		if (se) {
			offset += se->size;
		} else {
			eprintf ("r_bin_java_stack_map_frame_new: Unable to parse the Stack Items for the stack frame.\n");
			r_bin_java_stack_frame_free (stack_frame);
			return NULL;
		}
		r_list_append (stack_frame->stack_items, (void *) se);
		// Maybe?  3. Copy the previous frames locals and set the locals count.
		// copy_type_info_to_stack_frame_list_up_to_idx (p_frame->local_items, stack_frame->local_items, idx);
		if (p_frame) {
			stack_frame->number_of_locals = p_frame->number_of_locals;
		} else {
			IFDBG eprintf ("Unable to set previous stackframe with the number of locals (current info.code_attr.implicit_frame was probably not set :/)");
		}
		IFDBG eprintf ("r_bin_java_stack_map_frame_new: TODO Stack Frame Same Locals 1 Stack Element Condition is untested, so there may be issues.\n");
		break;
	case R_BIN_JAVA_STACK_FRAME_CHOP:
		// 1. Calculate the max index we want to copy from the list of the
		// previous frames locals
		IFDBG eprintf ("r_bin_java_stack_map_frame_new: Parsing R_BIN_JAVA_STACK_FRAME_CHOP.\n");
		// ut16 k = 251 - stack_frame->tag;
		/*,
		idx = p_frame->number_of_locals - k;
		*/
		// 2.  read the uoffset value
		stack_frame->offset_delta = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		// Maybe? 3. Copy the previous frames locals and set the locals count.
		// copy_type_info_to_stack_frame_list_up_to_idx (p_frame->local_items, stack_frame->local_items, idx);
		if (p_frame) {
			stack_frame->number_of_locals = p_frame->number_of_locals;
		} else {
			IFINT eprintf ("><?><\n");
			IFDBG eprintf ("Unable to set previous stackframe with the number of locals (current info.code_attr.implicit_frame was probably not set :/)");
		}
		IFDBG eprintf ("r_bin_java_stack_map_frame_new: TODO Stack Frame Chop Condition is untested, so there may be issues.\n");
		break;
	case R_BIN_JAVA_STACK_FRAME_SAME_FRAME_EXTENDED:
		IFDBG eprintf ("r_bin_java_stack_map_frame_new: Parsing R_BIN_JAVA_STACK_FRAME_SAME_FRAME_EXTENDED.\n");
		// 1. Read the uoffset
		stack_frame->offset_delta = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		// 2. Read the stack element type
		stack_frame->number_of_stack_items = 1;
		se = r_bin_java_read_from_buffer_verification_info_new (buffer + offset, sz - offset, buf_offset + offset);
		if (se) {
			offset += se->size;
		} else {
			eprintf ("r_bin_java_stack_map_frame_new: Unable to parse the Stack Items for the stack frame.\n");
			r_bin_java_stack_frame_free (stack_frame);
			return NULL;
		}
		r_list_append (stack_frame->stack_items, (void *) se);
		// Maybe? 3. Copy the previous frames locals to the current locals
		// copy_type_info_to_stack_frame_list_up_to_idx (p_frame->local_items, stack_frame->local_items, idx);
		if (p_frame) {
			stack_frame->number_of_locals = p_frame->number_of_locals;
		} else {
			IFINT eprintf ("><?><\n");
			IFDBG eprintf ("Unable to set previous stackframe with the number of locals (current info.code_attr.implicit_frame was probably not set :/)");
		}
		IFDBG eprintf ("r_bin_java_stack_map_frame_new: TODO Stack Frame Same Locals Frame Stack 1 Extended Condition is untested, so there may be issues.\n");
		break;
	case R_BIN_JAVA_STACK_FRAME_APPEND:
		IFDBG eprintf ("r_bin_java_stack_map_frame_new: Parsing R_BIN_JAVA_STACK_FRAME_APPEND.\n");
		// 1. Calculate the max index we want to copy from the list of the
		// previous frames locals
		ut16 k = stack_frame->tag - 251;
		ut32 i = 0;
		// 2. Read the uoffset
		stack_frame->offset_delta = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		// Maybe? 3. Copy the previous frames locals to the current locals
		// copy_type_info_to_stack_frame_list_up_to_idx (p_frame->local_items, stack_frame->local_items, idx);
		// 4. Read off the rest of the appended locals types
		for (i = 0; i < k; i++) {
			if (offset >= sz) {
				break;
			}
			IFDBG eprintf ("r_bin_java_stack_map_frame_new: Parsing verifying the k'th frame: %d of %d.\n", i, k);
			se = r_bin_java_read_from_buffer_verification_info_new (buffer + offset, sz - offset, buf_offset + offset);
			IFDBG eprintf ("r_bin_java_stack_map_frame_new: Completed Parsing\n");
			if (se) {
				offset += se->size;
			} else {
				eprintf ("r_bin_java_stack_map_frame_new: Unable to parse the locals for the stack frame.\n");
				r_bin_java_stack_frame_free (stack_frame);
				return NULL;
			}
			r_list_append (stack_frame->local_items, (void *) se);
		}
		IFDBG eprintf ("r_bin_java_stack_map_frame_new: Breaking out of loop");
		IFDBG eprintf ("p_frame: %p\n", p_frame);
		if (p_frame) {
			stack_frame->number_of_locals = p_frame->number_of_locals + k;
		} else {
			IFINT eprintf ("><?><\n");
			IFDBG eprintf ("Unable to set previous stackframe with the number of locals (current info.code_attr.implicit_frame was probably not set :/)");
		}
		IFDBG eprintf ("r_bin_java_stack_map_frame_new: TODO Stack Frame Same Locals Frame Stack 1 Extended Condition is untested, so there may be issues.\n");
		break;
	case R_BIN_JAVA_STACK_FRAME_FULL_FRAME:
		IFDBG eprintf ("r_bin_java_stack_map_frame_new: Parsing R_BIN_JAVA_STACK_FRAME_FULL_FRAME.\n");
		stack_frame->offset_delta = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		// IFDBG eprintf ("r_bin_java_stack_map_frame_new: Code Size > 65535, read(%d byte(s)), offset = 0x%08x.\n", var_sz, stack_frame->offset_delta);
		// Read the number of variables based on the max # local variable
		stack_frame->number_of_locals = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		// IFDBG eprintf ("r_bin_java_stack_map_frame_new: Max ulocalvar > 65535, read(%d byte(s)), number_of_locals = 0x%08x.\n", var_sz, stack_frame->number_of_locals);
		IFDBG r_bin_java_print_stack_map_frame_summary(stack_frame);
		// read the number of locals off the stack
		for (i = 0; i < stack_frame->number_of_locals; i++) {
			if (offset >= sz) {
				break;
			}
			se = r_bin_java_read_from_buffer_verification_info_new (buffer + offset, sz - offset, buf_offset + offset);
			if (se) {
				offset += se->size;
				// r_list_append (stack_frame->local_items, (void *) se);
			} else {
				eprintf ("r_bin_java_stack_map_frame_new: Unable to parse the locals for the stack frame.\n");
				r_bin_java_stack_frame_free (stack_frame);
				return NULL;
			}
			r_list_append (stack_frame->local_items, (void *) se);
		}
		// Read the number of stack items based on the max size of stack
		stack_frame->number_of_stack_items = R_BIN_JAVA_USHORT (buffer, offset);
		offset += 2;
		// IFDBG eprintf ("r_bin_java_stack_map_frame_new: Max ustack items > 65535, read(%d byte(s)), number_of_locals = 0x%08x.\n", var_sz, stack_frame->number_of_stack_items);
		// read the stack items
		for (i = 0; i < stack_frame->number_of_stack_items; i++) {
			if (offset >= sz) {
				break;
			}
			se = r_bin_java_read_from_buffer_verification_info_new (buffer + offset, sz - offset, buf_offset + offset);
			if (se) {
				offset += se->size;
				// r_list_append (stack_frame->stack_items, (void *) se);
			} else {
				eprintf ("r_bin_java_stack_map_frame_new: Unable to parse the stack items for the stack frame.\n");
				r_bin_java_stack_frame_free (stack_frame);
				return NULL;
			}
			r_list_append (stack_frame->local_items, (void *) se);
		}
		break;
	default:
		eprintf ("java: Unknown type\n");
		break;
	}
	// IFDBG eprintf ("Created a stack frame at offset(0x%08"PFMT64x") of size: %d\n", buf_offset, stack_frame->size);//r_bin_java_print_stack_map_frame_summary(stack_frame);
	stack_frame->size = offset;
	// IFDBG r_bin_java_print_stack_map_frame_summary(stack_frame);
	return stack_frame;
}