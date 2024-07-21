bool Item_default_value::fix_fields(THD *thd, Item **items)
{
  Item *real_arg;
  Item_field *field_arg;
  Field *def_field;
  DBUG_ASSERT(fixed == 0);
  DBUG_ASSERT(arg);

  /*
    DEFAULT() do not need table field so should not ask handler to bring
    field value (mark column for read)
  */
  enum_column_usage save_column_usage= thd->column_usage;
  /*
    Fields which has defult value could be read, so it is better hide system
    invisible columns.
  */
  thd->column_usage= COLUMNS_WRITE;
  if (arg->fix_fields_if_needed(thd, &arg))
  {
    thd->column_usage= save_column_usage;
    goto error;
  }
  thd->column_usage= save_column_usage;

  real_arg= arg->real_item();
  if (real_arg->type() != FIELD_ITEM)
  {
    my_error(ER_NO_DEFAULT_FOR_FIELD, MYF(0), arg->name.str);
    goto error;
  }

  field_arg= (Item_field *)real_arg;
  if ((field_arg->field->flags & NO_DEFAULT_VALUE_FLAG))
  {
    my_error(ER_NO_DEFAULT_FOR_FIELD, MYF(0),
             field_arg->field->field_name.str);
    goto error;
  }
  if (!(def_field= (Field*) thd->alloc(field_arg->field->size_of())))
    goto error;
  cached_field= def_field;
  memcpy((void *)def_field, (void *)field_arg->field,
         field_arg->field->size_of());
  def_field->reset_fields();
  // If non-constant default value expression or a blob
  if (def_field->default_value &&
      (def_field->default_value->flags || (def_field->flags & BLOB_FLAG)))
  {
    uchar *newptr= (uchar*) thd->alloc(1+def_field->pack_length());
    if (!newptr)
      goto error;
    /*
      Even if DEFAULT() do not read tables fields, the default value
      expression can do it.
    */
    if (def_field->default_value->fix_session_expr_for_read(thd, def_field))
      goto error;
    if (should_mark_column(thd->column_usage))
      def_field->default_value->expr->update_used_tables();
    def_field->move_field(newptr+1, def_field->maybe_null() ? newptr : 0, 1);
  }
  else
    def_field->move_field_offset((my_ptrdiff_t)
                                 (def_field->table->s->default_values -
                                  def_field->table->record[0]));
  set_field(def_field);
  return FALSE;

error:
  context->process_error(thd);
  return TRUE;
}