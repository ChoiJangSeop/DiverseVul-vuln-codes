int TABLE::update_virtual_field(Field *vf)
{
  DBUG_ENTER("TABLE::update_virtual_field");
  Query_arena backup_arena;
  Counting_error_handler count_errors;
  in_use->push_internal_handler(&count_errors);
  in_use->set_n_backup_active_arena(expr_arena, &backup_arena);
  bitmap_clear_all(&tmp_set);
  vf->vcol_info->expr->walk(&Item::update_vcol_processor, 0, &tmp_set);
  vf->vcol_info->expr->save_in_field(vf, 0);
  in_use->restore_active_arena(expr_arena, &backup_arena);
  in_use->pop_internal_handler();
  DBUG_RETURN(count_errors.errors);
}