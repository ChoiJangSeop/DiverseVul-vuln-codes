int TABLE::update_virtual_fields(handler *h, enum_vcol_update_mode update_mode)
{
  DBUG_ENTER("TABLE::update_virtual_fields");
  DBUG_PRINT("enter", ("update_mode: %d", update_mode));
  Field **vfield_ptr, *vf;
  Query_arena backup_arena;
  Turn_errors_to_warnings_handler Suppress_errors;
  int error;
  bool handler_pushed= 0, update_all_columns= 1;
  DBUG_ASSERT(vfield);

  if (h->keyread_enabled())
    DBUG_RETURN(0);

  error= 0;
  in_use->set_n_backup_active_arena(expr_arena, &backup_arena);

  /* When reading or deleting row, ignore errors from virtual columns */
  if (update_mode == VCOL_UPDATE_FOR_READ ||
      update_mode == VCOL_UPDATE_FOR_DELETE ||
      update_mode == VCOL_UPDATE_INDEXED)
  {
    in_use->push_internal_handler(&Suppress_errors);
    handler_pushed= 1;
  }
  else if (update_mode == VCOL_UPDATE_FOR_REPLACE &&
           in_use->is_current_stmt_binlog_format_row() &&
           in_use->variables.binlog_row_image != BINLOG_ROW_IMAGE_MINIMAL)
  {
    /*
      If we are doing a replace with not minimal binary logging, we have to
      calculate all virtual columns.
    */
    update_all_columns= 1;
  }

  /* Iterate over virtual fields in the table */
  for (vfield_ptr= vfield; *vfield_ptr; vfield_ptr++)
  {
    vf= (*vfield_ptr);
    Virtual_column_info *vcol_info= vf->vcol_info;
    DBUG_ASSERT(vcol_info);
    DBUG_ASSERT(vcol_info->expr);

    bool update= 0, swap_values= 0;
    switch (update_mode) {
    case VCOL_UPDATE_FOR_READ:
      update= (!vcol_info->stored_in_db &&
               bitmap_is_set(vcol_set, vf->field_index));
      swap_values= 1;
      break;
    case VCOL_UPDATE_FOR_DELETE:
    case VCOL_UPDATE_FOR_WRITE:
      update= bitmap_is_set(vcol_set, vf->field_index);
      break;
    case VCOL_UPDATE_FOR_REPLACE:
      update= ((!vcol_info->stored_in_db &&
                (vf->flags & (PART_KEY_FLAG | PART_INDIRECT_KEY_FLAG)) &&
                bitmap_is_set(vcol_set, vf->field_index)) ||
               update_all_columns);
      if (update && (vf->flags & BLOB_FLAG))
      {
        /*
          The row has been read into record[1] and Field_blob::value
          contains the value for record[0].  Swap value and read_value
          to ensure that the virtual column data for the read row will
          be in read_value at the end of this function
        */
        ((Field_blob*) vf)->swap_value_and_read_value();
        /* Ensure we call swap_value_and_read_value() after update */
        swap_values= 1;
      }
      break;
    case VCOL_UPDATE_INDEXED:
    case VCOL_UPDATE_INDEXED_FOR_UPDATE:
      /* Read indexed fields that was not updated in VCOL_UPDATE_FOR_READ */
      update= (!vcol_info->stored_in_db &&
               (vf->flags & (PART_KEY_FLAG | PART_INDIRECT_KEY_FLAG)) &&
               !bitmap_is_set(vcol_set, vf->field_index));
      swap_values= 1;
      break;
    }

    if (update)
    {
      int field_error __attribute__((unused)) = 0;
      /* Compute the actual value of the virtual fields */
      if (vcol_info->expr->save_in_field(vf, 0))
        field_error= error= 1;
      DBUG_PRINT("info", ("field '%s' - updated  error: %d",
                          vf->field_name.str, field_error));
      if (swap_values && (vf->flags & BLOB_FLAG))
      {
        /*
          Remember the read value to allow other update_virtual_field() calls
          for the same blob field for the row to be updated.
          Field_blob->read_value always contains the virtual column data for
          any read row.
        */
        ((Field_blob*) vf)->swap_value_and_read_value();
      }
    }
    else
    {
      DBUG_PRINT("info", ("field '%s' - skipped", vf->field_name.str));
    }
  }
  if (handler_pushed)
    in_use->pop_internal_handler();
  in_use->restore_active_arena(expr_arena, &backup_arena);
  
  /* Return 1 only of we got a fatal error, not a warning */
  DBUG_RETURN(in_use->is_error());
}