unpack_vcol_info_from_frm(THD *thd, MEM_ROOT *mem_root, TABLE *table,
                          String *expr_str, Virtual_column_info **vcol_ptr,
                          bool *error_reported)
{
  Create_field vcol_storage; // placeholder for vcol_info
  Parser_state parser_state;
  Virtual_column_info *vcol= *vcol_ptr, *vcol_info= 0;
  LEX *old_lex= thd->lex;
  LEX lex;
  bool error;
  DBUG_ENTER("unpack_vcol_info_from_frm");

  DBUG_ASSERT(vcol->expr == NULL);
  
  if (parser_state.init(thd, expr_str->c_ptr_safe(), expr_str->length()))
    goto end;

  if (init_lex_with_single_table(thd, table, &lex))
    goto end;

  lex.parse_vcol_expr= true;
  lex.last_field= &vcol_storage;

  error= parse_sql(thd, &parser_state, NULL);
  if (unlikely(error))
    goto end;

  if (lex.current_select->table_list.first[0].next_global)
  {
    /* We are using NEXT VALUE FOR sequence. Remember table name for open */
    TABLE_LIST *sequence= lex.current_select->table_list.first[0].next_global;
    sequence->next_global= table->internal_tables;
    table->internal_tables= sequence;
  }

  vcol_storage.vcol_info->set_vcol_type(vcol->get_vcol_type());
  vcol_storage.vcol_info->stored_in_db=      vcol->stored_in_db;
  vcol_storage.vcol_info->name=              vcol->name;
  vcol_storage.vcol_info->utf8=              vcol->utf8;
  if (!fix_and_check_vcol_expr(thd, table, vcol_storage.vcol_info))
  {
    *vcol_ptr= vcol_info= vcol_storage.vcol_info;   // Expression ok
    DBUG_ASSERT(vcol_info->expr);
    goto end;
  }
  *error_reported= TRUE;

end:
  end_lex_with_single_table(thd, table, old_lex);

  DBUG_RETURN(vcol_info);
}