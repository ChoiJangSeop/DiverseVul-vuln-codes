uint prep_alter_part_table(THD *thd, TABLE *table, Alter_info *alter_info,
                           HA_CREATE_INFO *create_info,
                           Alter_table_ctx *alter_ctx,
                           bool *partition_changed,
                           bool *fast_alter_table)
{
  DBUG_ENTER("prep_alter_part_table");

  /* Foreign keys on partitioned tables are not supported, waits for WL#148 */
  if (table->part_info && (alter_info->flags & (ALTER_ADD_FOREIGN_KEY |
                                                ALTER_DROP_FOREIGN_KEY)))
  {
    my_error(ER_FOREIGN_KEY_ON_PARTITIONED, MYF(0));
    DBUG_RETURN(TRUE);
  }
  /* Remove partitioning on a not partitioned table is not possible */
  if (!table->part_info && (alter_info->partition_flags &
                            ALTER_PARTITION_REMOVE))
  {
    my_error(ER_PARTITION_MGMT_ON_NONPARTITIONED, MYF(0));
    DBUG_RETURN(TRUE);
  }

  partition_info *alt_part_info= thd->lex->part_info;
  /*
    This variable is TRUE in very special case when we add only DEFAULT
    partition to the existing table
  */
  bool only_default_value_added=
    (alt_part_info &&
     alt_part_info->current_partition &&
     alt_part_info->current_partition->list_val_list.elements == 1 &&
     alt_part_info->current_partition->list_val_list.head()->
     added_items >= 1 &&
     alt_part_info->current_partition->list_val_list.head()->
     col_val_array[0].max_value) &&
    alt_part_info->part_type == LIST_PARTITION &&
    (alter_info->partition_flags & ALTER_PARTITION_ADD);
  if (only_default_value_added &&
      !thd->lex->part_info->num_columns)
    thd->lex->part_info->num_columns= 1; // to make correct clone

  /*
    One of these is done in handle_if_exists_option():
        thd->work_part_info= thd->lex->part_info;
      or
        thd->work_part_info= NULL;
  */
  if (thd->work_part_info &&
      !(thd->work_part_info= thd->work_part_info->get_clone(thd)))
    DBUG_RETURN(TRUE);

  /* ALTER_PARTITION_ADMIN is handled in mysql_admin_table */
  DBUG_ASSERT(!(alter_info->partition_flags & ALTER_PARTITION_ADMIN));

  partition_info *saved_part_info= NULL;

  if (alter_info->partition_flags &
      (ALTER_PARTITION_ADD |
       ALTER_PARTITION_DROP |
       ALTER_PARTITION_COALESCE |
       ALTER_PARTITION_REORGANIZE |
       ALTER_PARTITION_TABLE_REORG |
       ALTER_PARTITION_REBUILD))
  {
    /*
      You can't add column when we are doing alter related to partition
    */
    DBUG_EXECUTE_IF("test_pseudo_invisible", {
         my_error(ER_INTERNAL_ERROR, MYF(0), "Don't to it with test_pseudo_invisible");
         DBUG_RETURN(1);
         });
    DBUG_EXECUTE_IF("test_completely_invisible", {
         my_error(ER_INTERNAL_ERROR, MYF(0), "Don't to it with test_completely_invisible");
         DBUG_RETURN(1);
         });
    partition_info *tab_part_info;
    ulonglong flags= 0;
    bool is_last_partition_reorged= FALSE;
    part_elem_value *tab_max_elem_val= NULL;
    part_elem_value *alt_max_elem_val= NULL;
    longlong tab_max_range= 0, alt_max_range= 0;
    alt_part_info= thd->work_part_info;

    if (!table->part_info)
    {
      my_error(ER_PARTITION_MGMT_ON_NONPARTITIONED, MYF(0));
      DBUG_RETURN(TRUE);
    }

    /*
      Open our intermediate table, we will operate on a temporary instance
      of the original table, to be able to skip copying all partitions.
      Open it as a copy of the original table, and modify its partition_info
      object to allow fast_alter_partition_table to perform the changes.
    */
    DBUG_ASSERT(thd->mdl_context.is_lock_owner(MDL_key::TABLE,
                                               alter_ctx->db.str,
                                               alter_ctx->table_name.str,
                                               MDL_INTENTION_EXCLUSIVE));

    tab_part_info= table->part_info;

    if (alter_info->partition_flags & ALTER_PARTITION_TABLE_REORG)
    {
      uint new_part_no, curr_part_no;
      /*
        'ALTER TABLE t REORG PARTITION' only allowed with auto partition
         if default partitioning is used.
      */

      if (tab_part_info->part_type != HASH_PARTITION ||
          ((table->s->db_type()->partition_flags() & HA_USE_AUTO_PARTITION) &&
           !tab_part_info->use_default_num_partitions) ||
          ((!(table->s->db_type()->partition_flags() & HA_USE_AUTO_PARTITION)) &&
           tab_part_info->use_default_num_partitions))
      {
        my_error(ER_REORG_NO_PARAM_ERROR, MYF(0));
        goto err;
      }
      new_part_no= table->file->get_default_no_partitions(create_info);
      curr_part_no= tab_part_info->num_parts;
      if (new_part_no == curr_part_no)
      {
        /*
          No change is needed, we will have the same number of partitions
          after the change as before. Thus we can reply ok immediately
          without any changes at all.
        */
        flags= table->file->alter_table_flags(alter_info->flags);
        if (flags & (HA_FAST_CHANGE_PARTITION | HA_PARTITION_ONE_PHASE))
        {
          *fast_alter_table= true;
          /* Force table re-open for consistency with the main case. */
          table->mark_table_for_reopen();
        }
        else
        {
          /*
            Create copy of partition_info to avoid modifying original
            TABLE::part_info, to keep it safe for later use.
          */
          if (!(tab_part_info= tab_part_info->get_clone(thd)))
            DBUG_RETURN(TRUE);
        }

        thd->work_part_info= tab_part_info;
        DBUG_RETURN(FALSE);
      }
      else if (new_part_no > curr_part_no)
      {
        /*
          We will add more partitions, we use the ADD PARTITION without
          setting the flag for no default number of partitions
        */
        alter_info->partition_flags|= ALTER_PARTITION_ADD;
        thd->work_part_info->num_parts= new_part_no - curr_part_no;
      }
      else
      {
        /*
          We will remove hash partitions, we use the COALESCE PARTITION
          without setting the flag for no default number of partitions
        */
        alter_info->partition_flags|= ALTER_PARTITION_COALESCE;
        alter_info->num_parts= curr_part_no - new_part_no;
      }
    }
    if (!(flags= table->file->alter_table_flags(alter_info->flags)))
    {
      my_error(ER_PARTITION_FUNCTION_FAILURE, MYF(0));
      goto err;
    }
    if ((flags & (HA_FAST_CHANGE_PARTITION | HA_PARTITION_ONE_PHASE)) != 0)
    {
      /*
        "Fast" change of partitioning is supported in this case.
        We will change TABLE::part_info (as this is how we pass
        information to storage engine in this case), so the table
        must be reopened.
      */
      *fast_alter_table= true;
      table->mark_table_for_reopen();
    }
    else
    {
      /*
        "Fast" changing of partitioning is not supported. Create
        a copy of TABLE::part_info object, so we can modify it safely.
        Modifying original TABLE::part_info will cause problems when
        we read data from old version of table using this TABLE object
        while copying them to new version of table.
      */
      if (!(tab_part_info= tab_part_info->get_clone(thd)))
        DBUG_RETURN(TRUE);
    }
    DBUG_PRINT("info", ("*fast_alter_table flags: 0x%llx", flags));
    if ((alter_info->partition_flags & ALTER_PARTITION_ADD) ||
        (alter_info->partition_flags & ALTER_PARTITION_REORGANIZE))
    {
      if (thd->work_part_info->part_type != tab_part_info->part_type)
      {
        if (thd->work_part_info->part_type == NOT_A_PARTITION)
        {
          if (tab_part_info->part_type == RANGE_PARTITION)
          {
            my_error(ER_PARTITIONS_MUST_BE_DEFINED_ERROR, MYF(0), "RANGE");
            goto err;
          }
          else if (tab_part_info->part_type == LIST_PARTITION)
          {
            my_error(ER_PARTITIONS_MUST_BE_DEFINED_ERROR, MYF(0), "LIST");
            goto err;
          }
          /*
            Hash partitions can be altered without parser finds out about
            that it is HASH partitioned. So no error here.
          */
        }
        else
        {
          if (thd->work_part_info->part_type == RANGE_PARTITION)
          {
            my_error(ER_PARTITION_WRONG_VALUES_ERROR, MYF(0),
                     "RANGE", "LESS THAN");
          }
          else if (thd->work_part_info->part_type == LIST_PARTITION)
          {
            DBUG_ASSERT(thd->work_part_info->part_type == LIST_PARTITION);
            my_error(ER_PARTITION_WRONG_VALUES_ERROR, MYF(0),
                     "LIST", "IN");
          }
          else if (thd->work_part_info->part_type == VERSIONING_PARTITION ||
                   tab_part_info->part_type == VERSIONING_PARTITION)
          {
            my_error(ER_PARTITION_WRONG_TYPE, MYF(0), "SYSTEM_TIME");
          }
          else
          {
            DBUG_ASSERT(tab_part_info->part_type == RANGE_PARTITION ||
                        tab_part_info->part_type == LIST_PARTITION);
            (void) tab_part_info->error_if_requires_values();
          }
          goto err;
        }
      }
      if ((tab_part_info->column_list &&
          alt_part_info->num_columns != tab_part_info->num_columns &&
          !only_default_value_added) ||
          (!tab_part_info->column_list &&
            (tab_part_info->part_type == RANGE_PARTITION ||
             tab_part_info->part_type == LIST_PARTITION) &&
            alt_part_info->num_columns != 1U &&
             !only_default_value_added) ||
          (!tab_part_info->column_list &&
            tab_part_info->part_type == HASH_PARTITION &&
            (alt_part_info->num_columns != 0)))
      {
        my_error(ER_PARTITION_COLUMN_LIST_ERROR, MYF(0));
        goto err;
      }
      alt_part_info->column_list= tab_part_info->column_list;
      if (alt_part_info->fix_parser_data(thd))
      {
        goto err;
      }
    }
    if (alter_info->partition_flags & ALTER_PARTITION_ADD)
    {
      if (*fast_alter_table && thd->locked_tables_mode)
      {
        MEM_ROOT *old_root= thd->mem_root;
        thd->mem_root= &thd->locked_tables_list.m_locked_tables_root;
        saved_part_info= tab_part_info->get_clone(thd);
        thd->mem_root= old_root;
        saved_part_info->read_partitions= tab_part_info->read_partitions;
        saved_part_info->lock_partitions= tab_part_info->lock_partitions;
        saved_part_info->bitmaps_are_initialized= tab_part_info->bitmaps_are_initialized;
      }
      /*
        We start by moving the new partitions to the list of temporary
        partitions. We will then check that the new partitions fit in the
        partitioning scheme as currently set-up.
        Partitions are always added at the end in ADD PARTITION.
      */
      uint num_new_partitions= alt_part_info->num_parts;
      uint num_orig_partitions= tab_part_info->num_parts;
      uint check_total_partitions= num_new_partitions + num_orig_partitions;
      uint new_total_partitions= check_total_partitions;
      /*
        We allow quite a lot of values to be supplied by defaults, however we
        must know the number of new partitions in this case.
      */
      if (thd->lex->no_write_to_binlog &&
          tab_part_info->part_type != HASH_PARTITION &&
          tab_part_info->part_type != VERSIONING_PARTITION)
      {
        my_error(ER_NO_BINLOG_ERROR, MYF(0));
        goto err;
      }
      if (tab_part_info->defined_max_value &&
          (tab_part_info->part_type == RANGE_PARTITION ||
           alt_part_info->defined_max_value))
      {
        my_error((tab_part_info->part_type == RANGE_PARTITION ?
                  ER_PARTITION_MAXVALUE_ERROR :
                  ER_PARTITION_DEFAULT_ERROR), MYF(0));
        goto err;
      }
      if (num_new_partitions == 0)
      {
        my_error(ER_ADD_PARTITION_NO_NEW_PARTITION, MYF(0));
        goto err;
      }
      if (tab_part_info->is_sub_partitioned())
      {
        if (alt_part_info->num_subparts == 0)
          alt_part_info->num_subparts= tab_part_info->num_subparts;
        else if (alt_part_info->num_subparts != tab_part_info->num_subparts)
        {
          my_error(ER_ADD_PARTITION_SUBPART_ERROR, MYF(0));
          goto err;
        }
        check_total_partitions= new_total_partitions*
                                alt_part_info->num_subparts;
      }
      if (check_total_partitions > MAX_PARTITIONS)
      {
        my_error(ER_TOO_MANY_PARTITIONS_ERROR, MYF(0));
        goto err;
      }
      alt_part_info->part_type= tab_part_info->part_type;
      alt_part_info->subpart_type= tab_part_info->subpart_type;
      if (alt_part_info->set_up_defaults_for_partitioning(thd, table->file, 0,
                                                    tab_part_info->num_parts))
      {
        goto err;
      }
/*
Handling of on-line cases:

ADD PARTITION for RANGE/LIST PARTITIONING:
------------------------------------------
For range and list partitions add partition is simply adding a
new empty partition to the table. If the handler support this we
will use the simple method of doing this. The figure below shows
an example of this and the states involved in making this change.
            
Existing partitions                                     New added partitions
------       ------        ------        ------      |  ------    ------
|    |       |    |        |    |        |    |      |  |    |    |    |
| p0 |       | p1 |        | p2 |        | p3 |      |  | p4 |    | p5 |
------       ------        ------        ------      |  ------    ------
PART_NORMAL  PART_NORMAL   PART_NORMAL   PART_NORMAL    PART_TO_BE_ADDED*2
PART_NORMAL  PART_NORMAL   PART_NORMAL   PART_NORMAL    PART_IS_ADDED*2

The first line is the states before adding the new partitions and the 
second line is after the new partitions are added. All the partitions are
in the partitions list, no partitions are placed in the temp_partitions
list.

ADD PARTITION for HASH PARTITIONING
-----------------------------------
This little figure tries to show the various partitions involved when
adding two new partitions to a linear hash based partitioned table with
four partitions to start with, which lists are used and the states they
pass through. Adding partitions to a normal hash based is similar except
that it is always all the existing partitions that are reorganised not
only a subset of them.

Existing partitions                                     New added partitions
------       ------        ------        ------      |  ------    ------
|    |       |    |        |    |        |    |      |  |    |    |    |
| p0 |       | p1 |        | p2 |        | p3 |      |  | p4 |    | p5 |
------       ------        ------        ------      |  ------    ------
PART_CHANGED PART_CHANGED  PART_NORMAL   PART_NORMAL    PART_TO_BE_ADDED
PART_IS_CHANGED*2          PART_NORMAL   PART_NORMAL    PART_IS_ADDED
PART_NORMAL  PART_NORMAL   PART_NORMAL   PART_NORMAL    PART_IS_ADDED

Reorganised existing partitions
------      ------
|    |      |    |
| p0'|      | p1'|
------      ------

p0 - p5 will be in the partitions list of partitions.
p0' and p1' will actually not exist as separate objects, there presence can
be deduced from the state of the partition and also the names of those
partitions can be deduced this way.

After adding the partitions and copying the partition data to p0', p1',
p4 and p5 from p0 and p1 the states change to adapt for the new situation
where p0 and p1 is dropped and replaced by p0' and p1' and the new p4 and
p5 are in the table again.

The first line above shows the states of the partitions before we start
adding and copying partitions, the second after completing the adding
and copying and finally the third line after also dropping the partitions
that are reorganised.
*/
      if (*fast_alter_table && tab_part_info->part_type == HASH_PARTITION)
      {
        uint part_no= 0, start_part= 1, start_sec_part= 1;
        uint end_part= 0, end_sec_part= 0;
        uint upper_2n= tab_part_info->linear_hash_mask + 1;
        uint lower_2n= upper_2n >> 1;
        bool all_parts= TRUE;
        if (tab_part_info->linear_hash_ind && num_new_partitions < upper_2n)
        {
          /*
            An analysis of which parts needs reorganisation shows that it is
            divided into two intervals. The first interval is those parts
            that are reorganised up until upper_2n - 1. From upper_2n and
            onwards it starts again from partition 0 and goes on until
            it reaches p(upper_2n - 1). If the last new partition reaches
            beyond upper_2n - 1 then the first interval will end with
            p(lower_2n - 1) and start with p(num_orig_partitions - lower_2n).
            If lower_2n partitions are added then p0 to p(lower_2n - 1) will
            be reorganised which means that the two interval becomes one
            interval at this point. Thus only when adding less than
            lower_2n partitions and going beyond a total of upper_2n we
            actually get two intervals.

            To exemplify this assume we have 6 partitions to start with and
            add 1, 2, 3, 5, 6, 7, 8, 9 partitions.
            The first to add after p5 is p6 = 110 in bit numbers. Thus we
            can see that 10 = p2 will be partition to reorganise if only one
            partition.
            If 2 partitions are added we reorganise [p2, p3]. Those two
            cases are covered by the second if part below.
            If 3 partitions are added we reorganise [p2, p3] U [p0,p0]. This
            part is covered by the else part below.
            If 5 partitions are added we get [p2,p3] U [p0, p2] = [p0, p3].
            This is covered by the first if part where we need the max check
            to here use lower_2n - 1.
            If 7 partitions are added we get [p2,p3] U [p0, p4] = [p0, p4].
            This is covered by the first if part but here we use the first
            calculated end_part.
            Finally with 9 new partitions we would also reorganise p6 if we
            used the method below but we cannot reorganise more partitions
            than what we had from the start and thus we simply set all_parts
            to TRUE. In this case we don't get into this if-part at all.
          */
          all_parts= FALSE;
          if (num_new_partitions >= lower_2n)
          {
            /*
              In this case there is only one interval since the two intervals
              overlap and this starts from zero to last_part_no - upper_2n
            */
            start_part= 0;
            end_part= new_total_partitions - (upper_2n + 1);
            end_part= max(lower_2n - 1, end_part);
          }
          else if (new_total_partitions <= upper_2n)
          {
            /*
              Also in this case there is only one interval since we are not
              going over a 2**n boundary
            */
            start_part= num_orig_partitions - lower_2n;
            end_part= start_part + (num_new_partitions - 1);
          }
          else
          {
            /* We have two non-overlapping intervals since we are not
               passing a 2**n border and we have not at least lower_2n
               new parts that would ensure that the intervals become
               overlapping.
            */
            start_part= num_orig_partitions - lower_2n;
            end_part= upper_2n - 1;
            start_sec_part= 0;
            end_sec_part= new_total_partitions - (upper_2n + 1);
          }
        }
        List_iterator<partition_element> tab_it(tab_part_info->partitions);
        part_no= 0;
        do
        {
          partition_element *p_elem= tab_it++;
          if (all_parts ||
              (part_no >= start_part && part_no <= end_part) ||
              (part_no >= start_sec_part && part_no <= end_sec_part))
          {
            p_elem->part_state= PART_CHANGED;
          }
        } while (++part_no < num_orig_partitions);
      }
      /*
        Need to concatenate the lists here to make it possible to check the
        partition info for correctness using check_partition_info.
        For on-line add partition we set the state of this partition to
        PART_TO_BE_ADDED to ensure that it is known that it is not yet
        usable (becomes usable when partition is created and the switch of
        partition configuration is made.
      */
      {
        partition_element *now_part= NULL;
        if (tab_part_info->part_type == VERSIONING_PARTITION)
        {
          List_iterator<partition_element> it(tab_part_info->partitions);
          partition_element *el;
          while ((el= it++))
          {
            if (el->type == partition_element::CURRENT)
            {
              it.remove();
              now_part= el;
            }
          }
          if (*fast_alter_table && tab_part_info->vers_info->interval.is_set())
          {
            partition_element *hist_part= tab_part_info->vers_info->hist_part;
            if (hist_part->range_value <= thd->query_start())
              hist_part->part_state= PART_CHANGED;
          }
        }
        List_iterator<partition_element> alt_it(alt_part_info->partitions);
        uint part_count= 0;
        do
        {
          partition_element *part_elem= alt_it++;
          if (*fast_alter_table)
            part_elem->part_state= PART_TO_BE_ADDED;
          if (unlikely(tab_part_info->partitions.push_back(part_elem,
                                                           thd->mem_root)))
            goto err;
        } while (++part_count < num_new_partitions);
        tab_part_info->num_parts+= num_new_partitions;
        if (tab_part_info->part_type == VERSIONING_PARTITION)
        {
          DBUG_ASSERT(now_part);
          if (unlikely(tab_part_info->partitions.push_back(now_part,
                                                           thd->mem_root)))
            goto err;
        }
      }
      /*
        If we specify partitions explicitly we don't use defaults anymore.
        Using ADD PARTITION also means that we don't have the default number
        of partitions anymore. We use this code also for Table reorganisations
        and here we don't set any default flags to FALSE.
      */
      if (!(alter_info->partition_flags & ALTER_PARTITION_TABLE_REORG))
      {
        if (!alt_part_info->use_default_partitions)
        {
          DBUG_PRINT("info", ("part_info: %p", tab_part_info));
          tab_part_info->use_default_partitions= FALSE;
        }
        tab_part_info->use_default_num_partitions= FALSE;
        tab_part_info->is_auto_partitioned= FALSE;
      }
    }
    else if (alter_info->partition_flags & ALTER_PARTITION_DROP)
    {
      /*
        Drop a partition from a range partition and list partitioning is
        always safe and can be made more or less immediate. It is necessary
        however to ensure that the partition to be removed is safely removed
        and that REPAIR TABLE can remove the partition if for some reason the
        command to drop the partition failed in the middle.
      */
      uint part_count= 0;
      uint num_parts_dropped= alter_info->partition_names.elements;
      uint num_parts_found= 0;
      List_iterator<partition_element> part_it(tab_part_info->partitions);

      tab_part_info->is_auto_partitioned= FALSE;
      if (tab_part_info->part_type == VERSIONING_PARTITION)
      {
        if (num_parts_dropped >= tab_part_info->num_parts - 1)
        {
          DBUG_ASSERT(table && table->s && table->s->table_name.str);
          my_error(ER_VERS_WRONG_PARTS, MYF(0), table->s->table_name.str);
          goto err;
        }
      }
      else
      {
        if (!(tab_part_info->part_type == RANGE_PARTITION ||
              tab_part_info->part_type == LIST_PARTITION))
        {
          my_error(ER_ONLY_ON_RANGE_LIST_PARTITION, MYF(0), "DROP");
          goto err;
        }
        if (num_parts_dropped >= tab_part_info->num_parts)
        {
          my_error(ER_DROP_LAST_PARTITION, MYF(0));
          goto err;
        }
      }
      do
      {
        partition_element *part_elem= part_it++;
        if (is_name_in_list(part_elem->partition_name,
                            alter_info->partition_names))
        {
          if (tab_part_info->part_type == VERSIONING_PARTITION)
          {
            if (part_elem->type == partition_element::CURRENT)
            {
              my_error(ER_VERS_WRONG_PARTS, MYF(0), table->s->table_name.str);
              goto err;
            }
            if (tab_part_info->vers_info->interval.is_set())
            {
              if (num_parts_found < part_count)
              {
                my_error(ER_VERS_DROP_PARTITION_INTERVAL, MYF(0));
                goto err;
              }
              tab_part_info->vers_info->interval.start=
                (my_time_t)part_elem->range_value;
            }
          }
          /*
            Set state to indicate that the partition is to be dropped.
          */
          num_parts_found++;
          part_elem->part_state= PART_TO_BE_DROPPED;
        }
      } while (++part_count < tab_part_info->num_parts);
      if (num_parts_found != num_parts_dropped)
      {
        my_error(ER_DROP_PARTITION_NON_EXISTENT, MYF(0), "DROP");
        goto err;
      }
      if (table->file->is_fk_defined_on_table_or_index(MAX_KEY))
      {
        my_error(ER_ROW_IS_REFERENCED, MYF(0));
        goto err;
      }
      tab_part_info->num_parts-= num_parts_dropped;
    }
    else if (alter_info->partition_flags & ALTER_PARTITION_REBUILD)
    {
      set_engine_all_partitions(tab_part_info,
                                tab_part_info->default_engine_type);
      if (set_part_state(alter_info, tab_part_info, PART_CHANGED))
      {
        my_error(ER_DROP_PARTITION_NON_EXISTENT, MYF(0), "REBUILD");
        goto err;
      }
      if (!(*fast_alter_table))
      {
        table->file->print_error(HA_ERR_WRONG_COMMAND, MYF(0));
        goto err;
      }
    }
    else if (alter_info->partition_flags & ALTER_PARTITION_COALESCE)
    {
      uint num_parts_coalesced= alter_info->num_parts;
      uint num_parts_remain= tab_part_info->num_parts - num_parts_coalesced;
      List_iterator<partition_element> part_it(tab_part_info->partitions);
      if (tab_part_info->part_type != HASH_PARTITION)
      {
        my_error(ER_COALESCE_ONLY_ON_HASH_PARTITION, MYF(0));
        goto err;
      }
      if (num_parts_coalesced == 0)
      {
        my_error(ER_COALESCE_PARTITION_NO_PARTITION, MYF(0));
        goto err;
      }
      if (num_parts_coalesced >= tab_part_info->num_parts)
      {
        my_error(ER_DROP_LAST_PARTITION, MYF(0));
        goto err;
      }
/*
Online handling:
COALESCE PARTITION:
-------------------
The figure below shows the manner in which partitions are handled when
performing an on-line coalesce partition and which states they go through
at start, after adding and copying partitions and finally after dropping
the partitions to drop. The figure shows an example using four partitions
to start with, using linear hash and coalescing one partition (always the
last partition).

Using linear hash then all remaining partitions will have a new reorganised
part.

Existing partitions                     Coalesced partition 
------       ------              ------   |      ------
|    |       |    |              |    |   |      |    |
| p0 |       | p1 |              | p2 |   |      | p3 |
------       ------              ------   |      ------
PART_NORMAL  PART_CHANGED        PART_NORMAL     PART_REORGED_DROPPED
PART_NORMAL  PART_IS_CHANGED     PART_NORMAL     PART_TO_BE_DROPPED
PART_NORMAL  PART_NORMAL         PART_NORMAL     PART_IS_DROPPED

Reorganised existing partitions
            ------
            |    |
            | p1'|
            ------

p0 - p3 is in the partitions list.
The p1' partition will actually not be in any list it is deduced from the
state of p1.
*/
      {
        uint part_count= 0, start_part= 1, start_sec_part= 1;
        uint end_part= 0, end_sec_part= 0;
        bool all_parts= TRUE;
        if (*fast_alter_table &&
            tab_part_info->linear_hash_ind)
        {
          uint upper_2n= tab_part_info->linear_hash_mask + 1;
          uint lower_2n= upper_2n >> 1;
          all_parts= FALSE;
          if (num_parts_coalesced >= lower_2n)
          {
            all_parts= TRUE;
          }
          else if (num_parts_remain >= lower_2n)
          {
            end_part= tab_part_info->num_parts - (lower_2n + 1);
            start_part= num_parts_remain - lower_2n;
          }
          else
          {
            start_part= 0;
            end_part= tab_part_info->num_parts - (lower_2n + 1);
            end_sec_part= (lower_2n >> 1) - 1;
            start_sec_part= end_sec_part - (lower_2n - (num_parts_remain + 1));
          }
        }
        do
        {
          partition_element *p_elem= part_it++;
          if (*fast_alter_table &&
              (all_parts ||
              (part_count >= start_part && part_count <= end_part) ||
              (part_count >= start_sec_part && part_count <= end_sec_part)))
            p_elem->part_state= PART_CHANGED;
          if (++part_count > num_parts_remain)
          {
            if (*fast_alter_table)
              p_elem->part_state= PART_REORGED_DROPPED;
            else
              part_it.remove();
          }
        } while (part_count < tab_part_info->num_parts);
        tab_part_info->num_parts= num_parts_remain;
      }
      if (!(alter_info->partition_flags & ALTER_PARTITION_TABLE_REORG))
      {
        tab_part_info->use_default_num_partitions= FALSE;
        tab_part_info->is_auto_partitioned= FALSE;
      }
    }
    else if (alter_info->partition_flags & ALTER_PARTITION_REORGANIZE)
    {
      /*
        Reorganise partitions takes a number of partitions that are next
        to each other (at least for RANGE PARTITIONS) and then uses those
        to create a set of new partitions. So data is copied from those
        partitions into the new set of partitions. Those new partitions
        can have more values in the LIST value specifications or less both
        are allowed. The ranges can be different but since they are 
        changing a set of consecutive partitions they must cover the same
        range as those changed from.
        This command can be used on RANGE and LIST partitions.
      */
      uint num_parts_reorged= alter_info->partition_names.elements;
      uint num_parts_new= thd->work_part_info->partitions.elements;
      uint check_total_partitions;

      tab_part_info->is_auto_partitioned= FALSE;
      if (num_parts_reorged > tab_part_info->num_parts)
      {
        my_error(ER_REORG_PARTITION_NOT_EXIST, MYF(0));
        goto err;
      }
      if (!(tab_part_info->part_type == RANGE_PARTITION ||
            tab_part_info->part_type == LIST_PARTITION) &&
           (num_parts_new != num_parts_reorged))
      {
        my_error(ER_REORG_HASH_ONLY_ON_SAME_NO, MYF(0));
        goto err;
      }
      if (tab_part_info->is_sub_partitioned() &&
          alt_part_info->num_subparts &&
          alt_part_info->num_subparts != tab_part_info->num_subparts)
      {
        my_error(ER_PARTITION_WRONG_NO_SUBPART_ERROR, MYF(0));
        goto err;
      }
      check_total_partitions= tab_part_info->num_parts + num_parts_new;
      check_total_partitions-= num_parts_reorged;
      if (check_total_partitions > MAX_PARTITIONS)
      {
        my_error(ER_TOO_MANY_PARTITIONS_ERROR, MYF(0));
        goto err;
      }
      alt_part_info->part_type= tab_part_info->part_type;
      alt_part_info->subpart_type= tab_part_info->subpart_type;
      alt_part_info->num_subparts= tab_part_info->num_subparts;
      DBUG_ASSERT(!alt_part_info->use_default_partitions);
      /* We specified partitions explicitly so don't use defaults anymore. */
      tab_part_info->use_default_partitions= FALSE;
      if (alt_part_info->set_up_defaults_for_partitioning(thd, table->file, 0,
                                                          0))
      {
        goto err;
      }
      check_datadir_altered_for_innodb(thd, tab_part_info, alt_part_info);

/*
Online handling:
REORGANIZE PARTITION:
---------------------
The figure exemplifies the handling of partitions, their state changes and
how they are organised. It exemplifies four partitions where two of the
partitions are reorganised (p1 and p2) into two new partitions (p4 and p5).
The reason of this change could be to change range limits, change list
values or for hash partitions simply reorganise the partition which could
also involve moving them to new disks or new node groups (MySQL Cluster).

Existing partitions                                  
------       ------        ------        ------
|    |       |    |        |    |        |    |
| p0 |       | p1 |        | p2 |        | p3 |
------       ------        ------        ------
PART_NORMAL  PART_TO_BE_REORGED          PART_NORMAL
PART_NORMAL  PART_TO_BE_DROPPED          PART_NORMAL
PART_NORMAL  PART_IS_DROPPED             PART_NORMAL

Reorganised new partitions (replacing p1 and p2)
------      ------
|    |      |    |
| p4 |      | p5 |
------      ------
PART_TO_BE_ADDED
PART_IS_ADDED
PART_IS_ADDED

All unchanged partitions and the new partitions are in the partitions list
in the order they will have when the change is completed. The reorganised
partitions are placed in the temp_partitions list. PART_IS_ADDED is only a
temporary state not written in the frm file. It is used to ensure we write
the generated partition syntax in a correct manner.
*/
      {
        List_iterator<partition_element> tab_it(tab_part_info->partitions);
        uint part_count= 0;
        bool found_first= FALSE;
        bool found_last= FALSE;
        uint drop_count= 0;
        do
        {
          partition_element *part_elem= tab_it++;
          is_last_partition_reorged= FALSE;
          if (is_name_in_list(part_elem->partition_name,
                              alter_info->partition_names))
          {
            is_last_partition_reorged= TRUE;
            drop_count++;
            if (tab_part_info->column_list)
            {
              List_iterator<part_elem_value> p(part_elem->list_val_list);
              tab_max_elem_val= p++;
            }
            else
              tab_max_range= part_elem->range_value;
            if (*fast_alter_table &&
                unlikely(tab_part_info->temp_partitions.
                         push_back(part_elem, thd->mem_root)))
              goto err;

            if (*fast_alter_table)
              part_elem->part_state= PART_TO_BE_REORGED;
            if (!found_first)
            {
              uint alt_part_count= 0;
              partition_element *alt_part_elem;
              List_iterator<partition_element>
                                 alt_it(alt_part_info->partitions);
              found_first= TRUE;
              do
              {
                alt_part_elem= alt_it++;
                if (tab_part_info->column_list)
                {
                  List_iterator<part_elem_value> p(alt_part_elem->list_val_list);
                  alt_max_elem_val= p++;
                }
                else
                  alt_max_range= alt_part_elem->range_value;

                if (*fast_alter_table)
                  alt_part_elem->part_state= PART_TO_BE_ADDED;
                if (alt_part_count == 0)
                  tab_it.replace(alt_part_elem);
                else
                  tab_it.after(alt_part_elem);
              } while (++alt_part_count < num_parts_new);
            }
            else if (found_last)
            {
              my_error(ER_CONSECUTIVE_REORG_PARTITIONS, MYF(0));
              goto err;
            }
            else
              tab_it.remove();
          }
          else
          {
            if (found_first)
              found_last= TRUE;
          }
        } while (++part_count < tab_part_info->num_parts);
        if (drop_count != num_parts_reorged)
        {
          my_error(ER_DROP_PARTITION_NON_EXISTENT, MYF(0), "REORGANIZE");
          goto err;
        }
        tab_part_info->num_parts= check_total_partitions;
      }
    }
    else
    {
      DBUG_ASSERT(FALSE);
    }
    *partition_changed= TRUE;
    thd->work_part_info= tab_part_info;
    if (alter_info->partition_flags & (ALTER_PARTITION_ADD |
                                       ALTER_PARTITION_REORGANIZE))
    {
      if (tab_part_info->use_default_subpartitions &&
          !alt_part_info->use_default_subpartitions)
      {
        tab_part_info->use_default_subpartitions= FALSE;
        tab_part_info->use_default_num_subpartitions= FALSE;
      }

      if (tab_part_info->check_partition_info(thd, (handlerton**)NULL,
                                              table->file, 0, alt_part_info))
      {
        goto err;
      }
      /*
        The check below needs to be performed after check_partition_info
        since this function "fixes" the item trees of the new partitions
        to reorganize into
      */
      if (alter_info->partition_flags == ALTER_PARTITION_REORGANIZE &&
          tab_part_info->part_type == RANGE_PARTITION &&
          ((is_last_partition_reorged &&
            (tab_part_info->column_list ?
             (partition_info_compare_column_values(
                              alt_max_elem_val->col_val_array,
                              tab_max_elem_val->col_val_array) < 0) :
             alt_max_range < tab_max_range)) ||
            (!is_last_partition_reorged &&
             (tab_part_info->column_list ?
              (partition_info_compare_column_values(
                              alt_max_elem_val->col_val_array,
                              tab_max_elem_val->col_val_array) != 0) :
              alt_max_range != tab_max_range))))
      {
        /*
          For range partitioning the total resulting range before and
          after the change must be the same except in one case. This is
          when the last partition is reorganised, in this case it is
          acceptable to increase the total range.
          The reason is that it is not allowed to have "holes" in the
          middle of the ranges and thus we should not allow to reorganise
          to create "holes".
        */
        my_error(ER_REORG_OUTSIDE_RANGE, MYF(0));
        goto err;
      }
    }
  } // ADD, DROP, COALESCE, REORGANIZE, TABLE_REORG, REBUILD
  else
  {
    /*
     When thd->lex->part_info has a reference to a partition_info the
     ALTER TABLE contained a definition of a partitioning.

     Case I:
       If there was a partition before and there is a new one defined.
       We use the new partitioning. The new partitioning is already
       defined in the correct variable so no work is needed to
       accomplish this.
       We do however need to update partition_changed to ensure that not
       only the frm file is changed in the ALTER TABLE command.

     Case IIa:
       There was a partitioning before and there is no new one defined.
       Also the user has not specified to remove partitioning explicitly.

       We use the old partitioning also for the new table. We do this
       by assigning the partition_info from the table loaded in
       open_table to the partition_info struct used by mysql_create_table
       later in this method.

     Case IIb:
       There was a partitioning before and there is no new one defined.
       The user has specified explicitly to remove partitioning

       Since the user has specified explicitly to remove partitioning
       we override the old partitioning info and create a new table using
       the specified engine.
       In this case the partition also is changed.

     Case III:
       There was no partitioning before altering the table, there is
       partitioning defined in the altered table. Use the new partitioning.
       No work needed since the partitioning info is already in the
       correct variable.

       In this case we discover one case where the new partitioning is using
       the same partition function as the default (PARTITION BY KEY or
       PARTITION BY LINEAR KEY with the list of fields equal to the primary
       key fields OR PARTITION BY [LINEAR] KEY() for tables without primary
       key)
       Also here partition has changed and thus a new table must be
       created.

     Case IV:
       There was no partitioning before and no partitioning defined.
       Obviously no work needed.
    */
    partition_info *tab_part_info= table->part_info;

    if (tab_part_info)
    {
      if (alter_info->partition_flags & ALTER_PARTITION_REMOVE)
      {
        DBUG_PRINT("info", ("Remove partitioning"));
        if (!(create_info->used_fields & HA_CREATE_USED_ENGINE))
        {
          DBUG_PRINT("info", ("No explicit engine used"));
          create_info->db_type= tab_part_info->default_engine_type;
        }
        DBUG_PRINT("info", ("New engine type: %s",
                   ha_resolve_storage_engine_name(create_info->db_type)));
        thd->work_part_info= NULL;
        *partition_changed= TRUE;
      }
      else if (!thd->work_part_info)
      {
        /*
          Retain partitioning but possibly with a new storage engine
          beneath.

          Create a copy of TABLE::part_info to be able to modify it freely.
        */
        if (!(tab_part_info= tab_part_info->get_clone(thd)))
          DBUG_RETURN(TRUE);
        thd->work_part_info= tab_part_info;
        if (create_info->used_fields & HA_CREATE_USED_ENGINE &&
            create_info->db_type != tab_part_info->default_engine_type)
        {
          /*
            Make sure change of engine happens to all partitions.
          */
          DBUG_PRINT("info", ("partition changed"));
          if (tab_part_info->is_auto_partitioned)
          {
            /*
              If the user originally didn't specify partitioning to be
              used we can remove it now.
            */
            thd->work_part_info= NULL;
          }
          else
          {
            /*
              Ensure that all partitions have the proper engine set-up
            */
            set_engine_all_partitions(thd->work_part_info,
                                      create_info->db_type);
          }
          *partition_changed= TRUE;
        }
      }
      /*
        Prohibit inplace when partitioned by primary key and the primary key is changed.
      */
      if (!*partition_changed &&
          tab_part_info->part_field_array &&
          !tab_part_info->part_field_list.elements &&
          table->s->primary_key != MAX_KEY)
      {

        if (alter_info->flags & (ALTER_DROP_SYSTEM_VERSIONING |
                                 ALTER_ADD_SYSTEM_VERSIONING))
        {
          *partition_changed= true;
        }
        else
        {
          KEY *primary_key= table->key_info + table->s->primary_key;
          List_iterator_fast<Alter_drop> drop_it(alter_info->drop_list);
          const char *primary_name= primary_key->name.str;
          const Alter_drop *drop;
          drop_it.rewind();
          while ((drop= drop_it++))
          {
            if (drop->type == Alter_drop::KEY &&
                0 == my_strcasecmp(system_charset_info, primary_name, drop->name))
              break;
          }
          if (drop)
            *partition_changed= TRUE;
        }
      }
    }
    if (thd->work_part_info)
    {
      partition_info *part_info= thd->work_part_info;
      bool is_native_partitioned= FALSE;
      /*
        Need to cater for engine types that can handle partition without
        using the partition handler.
      */
      if (part_info != tab_part_info)
      {
        if (part_info->fix_parser_data(thd))
        {
          goto err;
        }
        /*
          Compare the old and new part_info. If only key_algorithm
          change is done, don't consider it as changed partitioning (to avoid
          rebuild). This is to handle KEY (numeric_cols) partitioned tables
          created in 5.1. For more info, see bug#14521864.
        */
        if (alter_info->partition_flags != ALTER_PARTITION_INFO ||
            !table->part_info ||
            alter_info->algorithm(thd) !=
              Alter_info::ALTER_TABLE_ALGORITHM_INPLACE ||
            !table->part_info->has_same_partitioning(part_info))
        {
          DBUG_PRINT("info", ("partition changed"));
          *partition_changed= true;
        }
      }

      /*
        Set up partition default_engine_type either from the create_info
        or from the previus table
      */
      if (create_info->used_fields & HA_CREATE_USED_ENGINE)
        part_info->default_engine_type= create_info->db_type;
      else
      {
        if (tab_part_info)
          part_info->default_engine_type= tab_part_info->default_engine_type;
        else
          part_info->default_engine_type= create_info->db_type;
      }
      DBUG_ASSERT(part_info->default_engine_type &&
                  part_info->default_engine_type != partition_hton);
      if (check_native_partitioned(create_info, &is_native_partitioned,
                                   part_info, thd))
      {
        goto err;
      }
      if (!is_native_partitioned)
      {
        DBUG_ASSERT(create_info->db_type);
        create_info->db_type= partition_hton;
      }
    }
  }
  DBUG_RETURN(FALSE);
err:
  *fast_alter_table= false;
  if (saved_part_info)
    table->part_info= saved_part_info;
  DBUG_RETURN(TRUE);
}