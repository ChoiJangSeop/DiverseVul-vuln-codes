Field *create_tmp_field_from_field(THD *thd, Field *org_field,
                                   const char *name, TABLE *table,
                                   Item_field *item)
{
  Field *new_field;

  new_field= org_field->make_new_field(thd->mem_root, table,
                                       table == org_field->table);
  if (new_field)
  {
    new_field->init(table);
    new_field->orig_table= org_field->orig_table;
    if (item)
      item->result_field= new_field;
    else
      new_field->field_name= name;
    new_field->flags|= (org_field->flags & NO_DEFAULT_VALUE_FLAG);
    if (org_field->maybe_null() || (item && item->maybe_null))
      new_field->flags&= ~NOT_NULL_FLAG;	// Because of outer join
    if (org_field->type() == MYSQL_TYPE_VAR_STRING ||
        org_field->type() == MYSQL_TYPE_VARCHAR)
      table->s->db_create_options|= HA_OPTION_PACK_RECORD;
    else if (org_field->type() == FIELD_TYPE_DOUBLE)
      ((Field_double *) new_field)->not_fixed= TRUE;
    new_field->vcol_info= new_field->default_value=
      new_field->check_constraint= 0;
    new_field->cond_selectivity= 1.0;
    new_field->next_equal_field= NULL;
    new_field->option_list= NULL;
    new_field->option_struct= NULL;
  }
  return new_field;
}