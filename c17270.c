bool parse_vcol_defs(THD *thd, MEM_ROOT *mem_root, TABLE *table,
                     bool *error_reported, vcol_init_mode mode)
{
  CHARSET_INFO *save_character_set_client= thd->variables.character_set_client;
  CHARSET_INFO *save_collation= thd->variables.collation_connection;
  Query_arena  *backup_stmt_arena_ptr= thd->stmt_arena;
  const uchar *pos= table->s->vcol_defs.str;
  const uchar *end= pos + table->s->vcol_defs.length;
  Field **field_ptr= table->field - 1;
  Field **vfield_ptr= table->vfield;
  Field **dfield_ptr= table->default_field;
  Virtual_column_info **check_constraint_ptr= table->check_constraints;
  sql_mode_t saved_mode= thd->variables.sql_mode;
  Query_arena backup_arena;
  Virtual_column_info *vcol= 0;
  StringBuffer<MAX_FIELD_WIDTH> expr_str;
  bool res= 1;
  DBUG_ENTER("parse_vcol_defs");

  if (check_constraint_ptr)
    memcpy(table->check_constraints + table->s->field_check_constraints,
           table->s->check_constraints,
           table->s->table_check_constraints * sizeof(Virtual_column_info*));

  DBUG_ASSERT(table->expr_arena == NULL);
  /*
    We need to use CONVENTIONAL_EXECUTION here to ensure that
    any new items created by fix_fields() are not reverted.
  */
  table->expr_arena= new (alloc_root(mem_root, sizeof(Table_arena)))
                        Table_arena(mem_root, 
                                    Query_arena::STMT_CONVENTIONAL_EXECUTION);
  if (!table->expr_arena)
    DBUG_RETURN(1);

  thd->set_n_backup_active_arena(table->expr_arena, &backup_arena);
  thd->stmt_arena= table->expr_arena;
  thd->update_charset(&my_charset_utf8mb4_general_ci, table->s->table_charset);
  expr_str.append(&parse_vcol_keyword);
  thd->variables.sql_mode &= ~(MODE_NO_BACKSLASH_ESCAPES | MODE_EMPTY_STRING_IS_NULL);

  while (pos < end)
  {
    uint type, expr_length;
    if (table->s->frm_version >= FRM_VER_EXPRESSSIONS)
    {
      uint field_nr, name_length;
      /* see pack_expression() for how data is stored */
      type= pos[0];
      field_nr= uint2korr(pos+1);
      expr_length= uint2korr(pos+3);
      name_length= pos[5];
      pos+= FRM_VCOL_NEW_HEADER_SIZE + name_length;
      field_ptr= table->field + field_nr;
    }
    else
    {
      /*
        see below in ::init_from_binary_frm_image for how data is stored
        in versions below 10.2 (that includes 5.7 too)
      */
      while (*++field_ptr && !(*field_ptr)->vcol_info) /* no-op */;
      if (!*field_ptr)
      {
        open_table_error(table->s, OPEN_FRM_CORRUPTED, 1);
        goto end;
      }
      type= (*field_ptr)->vcol_info->stored_in_db
            ? VCOL_GENERATED_STORED : VCOL_GENERATED_VIRTUAL;
      expr_length= uint2korr(pos+1);
      if (table->s->mysql_version > 50700 && table->s->mysql_version < 100000)
        pos+= 4;                        // MySQL from 5.7
      else
        pos+= pos[0] == 2 ? 4 : 3;      // MariaDB from 5.2 to 10.1
    }

    expr_str.length(parse_vcol_keyword.length);
    expr_str.append((char*)pos, expr_length);
    thd->where= vcol_type_name(static_cast<enum_vcol_info_type>(type));

    switch (type) {
    case VCOL_GENERATED_VIRTUAL:
    case VCOL_GENERATED_STORED:
      vcol= unpack_vcol_info_from_frm(thd, mem_root, table, &expr_str,
                                    &((*field_ptr)->vcol_info), error_reported);
      *(vfield_ptr++)= *field_ptr;
      if (vcol && field_ptr[0]->check_vcol_sql_mode_dependency(thd, mode))
      {
        DBUG_ASSERT(thd->is_error());
        *error_reported= true;
        goto end;
      }
      break;
    case VCOL_DEFAULT:
      vcol= unpack_vcol_info_from_frm(thd, mem_root, table, &expr_str,
                                      &((*field_ptr)->default_value),
                                      error_reported);
      *(dfield_ptr++)= *field_ptr;
      if (vcol && (vcol->flags & (VCOL_NON_DETERMINISTIC | VCOL_SESSION_FUNC)))
        table->s->non_determinstic_insert= true;
      break;
    case VCOL_CHECK_FIELD:
      vcol= unpack_vcol_info_from_frm(thd, mem_root, table, &expr_str,
                                      &((*field_ptr)->check_constraint),
                                      error_reported);
      *check_constraint_ptr++= (*field_ptr)->check_constraint;
      break;
    case VCOL_CHECK_TABLE:
      vcol= unpack_vcol_info_from_frm(thd, mem_root, table, &expr_str,
                                      check_constraint_ptr, error_reported);
      check_constraint_ptr++;
      break;
    }
    if (!vcol)
      goto end;
    pos+= expr_length;
  }

  /* Now, initialize CURRENT_TIMESTAMP fields */
  for (field_ptr= table->field; *field_ptr; field_ptr++)
  {
    Field *field= *field_ptr;
    if (field->has_default_now_unireg_check())
    {
      expr_str.length(parse_vcol_keyword.length);
      expr_str.append(STRING_WITH_LEN("current_timestamp("));
      expr_str.append_ulonglong(field->decimals());
      expr_str.append(')');
      vcol= unpack_vcol_info_from_frm(thd, mem_root, table, &expr_str,
                                      &((*field_ptr)->default_value),
                                      error_reported);
      *(dfield_ptr++)= *field_ptr;
      if (!field->default_value->expr)
        goto end;
    }
    else if (field->has_update_default_function() && !field->default_value)
      *(dfield_ptr++)= *field_ptr;
  }

  if (vfield_ptr)
    *vfield_ptr= 0;

  if (dfield_ptr)
    *dfield_ptr= 0;

  if (check_constraint_ptr)
    *check_constraint_ptr= 0;

  /* Check that expressions aren't referring to not yet initialized fields */
  for (field_ptr= table->field; *field_ptr; field_ptr++)
  {
    Field *field= *field_ptr;
    if (check_vcol_forward_refs(field, field->vcol_info, 0) ||
        check_vcol_forward_refs(field, field->check_constraint, 1) ||
        check_vcol_forward_refs(field, field->default_value, 0))
    {
      *error_reported= true;
      goto end;
    }
  }

  res=0;
end:
  thd->restore_active_arena(table->expr_arena, &backup_arena);
  thd->stmt_arena= backup_stmt_arena_ptr;
  if (save_character_set_client)
    thd->update_charset(save_character_set_client, save_collation);
  thd->variables.sql_mode= saved_mode;
  DBUG_RETURN(res);
}