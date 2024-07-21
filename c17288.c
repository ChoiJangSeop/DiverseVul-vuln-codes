int Field::set_default()
{
  if (default_value)
  {
    Query_arena backup_arena;
    table->in_use->set_n_backup_active_arena(table->expr_arena, &backup_arena);
    int rc= default_value->expr->save_in_field(this, 0);
    table->in_use->restore_active_arena(table->expr_arena, &backup_arena);
    return rc;
  }
  /* Copy constant value stored in s->default_values */
  my_ptrdiff_t l_offset= (my_ptrdiff_t) (table->s->default_values -
                                        table->record[0]);
  memcpy(ptr, ptr + l_offset, pack_length_in_rec());
  if (maybe_null_in_table())
    *null_ptr= ((*null_ptr & (uchar) ~null_bit) |
                (null_ptr[l_offset] & null_bit));
  return 0;
}