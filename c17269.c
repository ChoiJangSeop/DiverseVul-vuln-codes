static bool mysql_drop_partitions(ALTER_PARTITION_PARAM_TYPE *lpt)
{
  char path[FN_REFLEN+1];
  partition_info *part_info= lpt->table->part_info;
  List_iterator<partition_element> part_it(part_info->partitions);
  uint i= 0;
  uint remove_count= 0;
  int error;
  DBUG_ENTER("mysql_drop_partitions");

  DBUG_ASSERT(lpt->thd->mdl_context.is_lock_owner(MDL_key::TABLE,
                                                lpt->table->s->db.str,
                                                lpt->table->s->table_name.str,
                                                MDL_EXCLUSIVE));

  build_table_filename(path, sizeof(path) - 1, lpt->db.str, lpt->table_name.str, "", 0);
  if ((error= lpt->table->file->ha_drop_partitions(path)))
  {
    lpt->table->file->print_error(error, MYF(0));
    DBUG_RETURN(TRUE);
  }
  do
  {
    partition_element *part_elem= part_it++;
    if (part_elem->part_state == PART_IS_DROPPED)
    {
      part_it.remove();
      remove_count++;
    }
  } while (++i < part_info->num_parts);
  part_info->num_parts-= remove_count;
  DBUG_RETURN(FALSE);
}