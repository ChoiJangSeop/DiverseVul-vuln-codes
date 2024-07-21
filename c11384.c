static void copy_recurse_data(compiler_common *common, PCRE2_SPTR cc, PCRE2_SPTR ccend,
  int type, int stackptr, int stacktop, BOOL has_quit)
{
delayed_mem_copy_status status;
PCRE2_SPTR alternative;
sljit_sw private_srcw[2];
sljit_sw shared_srcw[3];
sljit_sw kept_shared_srcw[2];
int private_count, shared_count, kept_shared_count;
int from_sp, base_reg, offset, i;
BOOL setsom_found = FALSE;
BOOL setmark_found = FALSE;
BOOL capture_last_found = FALSE;
BOOL control_head_found = FALSE;

#if defined DEBUG_FORCE_CONTROL_HEAD && DEBUG_FORCE_CONTROL_HEAD
SLJIT_ASSERT(common->control_head_ptr != 0);
control_head_found = TRUE;
#endif

switch (type)
  {
  case recurse_copy_from_global:
  from_sp = TRUE;
  base_reg = STACK_TOP;
  break;

  case recurse_copy_private_to_global:
  case recurse_copy_shared_to_global:
  case recurse_copy_kept_shared_to_global:
  from_sp = FALSE;
  base_reg = STACK_TOP;
  break;

  default:
  SLJIT_ASSERT(type == recurse_swap_global);
  from_sp = FALSE;
  base_reg = TMP2;
  break;
  }

stackptr = STACK(stackptr);
stacktop = STACK(stacktop);

status.tmp_regs[0] = TMP1;
status.saved_tmp_regs[0] = TMP1;

if (base_reg != TMP2)
  {
  status.tmp_regs[1] = TMP2;
  status.saved_tmp_regs[1] = TMP2;
  }
else
  {
  status.saved_tmp_regs[1] = RETURN_ADDR;
  if (HAS_VIRTUAL_REGISTERS)
    status.tmp_regs[1] = STR_PTR;
  else
    status.tmp_regs[1] = RETURN_ADDR;
  }

status.saved_tmp_regs[2] = TMP3;
if (HAS_VIRTUAL_REGISTERS)
  status.tmp_regs[2] = STR_END;
else
  status.tmp_regs[2] = TMP3;

delayed_mem_copy_init(&status, common);

if (type != recurse_copy_shared_to_global && type != recurse_copy_kept_shared_to_global)
  {
  SLJIT_ASSERT(type == recurse_copy_from_global || type == recurse_copy_private_to_global || type == recurse_swap_global);

  if (!from_sp)
    delayed_mem_copy_move(&status, base_reg, stackptr, SLJIT_SP, common->recursive_head_ptr);

  if (from_sp || type == recurse_swap_global)
    delayed_mem_copy_move(&status, SLJIT_SP, common->recursive_head_ptr, base_reg, stackptr);
  }

stackptr += sizeof(sljit_sw);

#if defined DEBUG_FORCE_CONTROL_HEAD && DEBUG_FORCE_CONTROL_HEAD
if (type != recurse_copy_shared_to_global)
  {
  if (!from_sp)
    delayed_mem_copy_move(&status, base_reg, stackptr, SLJIT_SP, common->control_head_ptr);

  if (from_sp || type == recurse_swap_global)
    delayed_mem_copy_move(&status, SLJIT_SP, common->control_head_ptr, base_reg, stackptr);
  }

stackptr += sizeof(sljit_sw);
#endif

while (cc < ccend)
  {
  private_count = 0;
  shared_count = 0;
  kept_shared_count = 0;

  switch(*cc)
    {
    case OP_SET_SOM:
    SLJIT_ASSERT(common->has_set_som);
    if (has_quit && !setsom_found)
      {
      kept_shared_srcw[0] = OVECTOR(0);
      kept_shared_count = 1;
      setsom_found = TRUE;
      }
    cc += 1;
    break;

    case OP_RECURSE:
    if (has_quit)
      {
      if (common->has_set_som && !setsom_found)
        {
        kept_shared_srcw[0] = OVECTOR(0);
        kept_shared_count = 1;
        setsom_found = TRUE;
        }
      if (common->mark_ptr != 0 && !setmark_found)
        {
        kept_shared_srcw[kept_shared_count] = common->mark_ptr;
        kept_shared_count++;
        setmark_found = TRUE;
        }
      }
    if (common->capture_last_ptr != 0 && !capture_last_found)
      {
      shared_srcw[0] = common->capture_last_ptr;
      shared_count = 1;
      capture_last_found = TRUE;
      }
    cc += 1 + LINK_SIZE;
    break;

    case OP_KET:
    if (PRIVATE_DATA(cc) != 0)
      {
      private_count = 1;
      private_srcw[0] = PRIVATE_DATA(cc);
      SLJIT_ASSERT(PRIVATE_DATA(cc + 1) != 0);
      cc += PRIVATE_DATA(cc + 1);
      }
    cc += 1 + LINK_SIZE;
    break;

    case OP_ASSERT:
    case OP_ASSERT_NOT:
    case OP_ASSERTBACK:
    case OP_ASSERTBACK_NOT:
    case OP_ASSERT_NA:
    case OP_ASSERTBACK_NA:
    case OP_ONCE:
    case OP_SCRIPT_RUN:
    case OP_BRAPOS:
    case OP_SBRA:
    case OP_SBRAPOS:
    case OP_SCOND:
    private_count = 1;
    private_srcw[0] = PRIVATE_DATA(cc);
    cc += 1 + LINK_SIZE;
    break;

    case OP_CBRA:
    case OP_SCBRA:
    offset = (GET2(cc, 1 + LINK_SIZE)) << 1;
    shared_srcw[0] = OVECTOR(offset);
    shared_srcw[1] = OVECTOR(offset + 1);
    shared_count = 2;

    if (common->capture_last_ptr != 0 && !capture_last_found)
      {
      shared_srcw[2] = common->capture_last_ptr;
      shared_count = 3;
      capture_last_found = TRUE;
      }

    if (common->optimized_cbracket[GET2(cc, 1 + LINK_SIZE)] == 0)
      {
      private_count = 1;
      private_srcw[0] = OVECTOR_PRIV(GET2(cc, 1 + LINK_SIZE));
      }
    cc += 1 + LINK_SIZE + IMM2_SIZE;
    break;

    case OP_CBRAPOS:
    case OP_SCBRAPOS:
    offset = (GET2(cc, 1 + LINK_SIZE)) << 1;
    shared_srcw[0] = OVECTOR(offset);
    shared_srcw[1] = OVECTOR(offset + 1);
    shared_count = 2;

    if (common->capture_last_ptr != 0 && !capture_last_found)
      {
      shared_srcw[2] = common->capture_last_ptr;
      shared_count = 3;
      capture_last_found = TRUE;
      }

    private_count = 2;
    private_srcw[0] = PRIVATE_DATA(cc);
    private_srcw[1] = OVECTOR_PRIV(GET2(cc, 1 + LINK_SIZE));
    cc += 1 + LINK_SIZE + IMM2_SIZE;
    break;

    case OP_COND:
    /* Might be a hidden SCOND. */
    alternative = cc + GET(cc, 1);
    if (*alternative == OP_KETRMAX || *alternative == OP_KETRMIN)
      {
      private_count = 1;
      private_srcw[0] = PRIVATE_DATA(cc);
      }
    cc += 1 + LINK_SIZE;
    break;

    CASE_ITERATOR_PRIVATE_DATA_1
    if (PRIVATE_DATA(cc))
      {
      private_count = 1;
      private_srcw[0] = PRIVATE_DATA(cc);
      }
    cc += 2;
#ifdef SUPPORT_UNICODE
    if (common->utf && HAS_EXTRALEN(cc[-1])) cc += GET_EXTRALEN(cc[-1]);
#endif
    break;

    CASE_ITERATOR_PRIVATE_DATA_2A
    if (PRIVATE_DATA(cc))
      {
      private_count = 2;
      private_srcw[0] = PRIVATE_DATA(cc);
      private_srcw[1] = PRIVATE_DATA(cc) + sizeof(sljit_sw);
      }
    cc += 2;
#ifdef SUPPORT_UNICODE
    if (common->utf && HAS_EXTRALEN(cc[-1])) cc += GET_EXTRALEN(cc[-1]);
#endif
    break;

    CASE_ITERATOR_PRIVATE_DATA_2B
    if (PRIVATE_DATA(cc))
      {
      private_count = 2;
      private_srcw[0] = PRIVATE_DATA(cc);
      private_srcw[1] = PRIVATE_DATA(cc) + sizeof(sljit_sw);
      }
    cc += 2 + IMM2_SIZE;
#ifdef SUPPORT_UNICODE
    if (common->utf && HAS_EXTRALEN(cc[-1])) cc += GET_EXTRALEN(cc[-1]);
#endif
    break;

    CASE_ITERATOR_TYPE_PRIVATE_DATA_1
    if (PRIVATE_DATA(cc))
      {
      private_count = 1;
      private_srcw[0] = PRIVATE_DATA(cc);
      }
    cc += 1;
    break;

    CASE_ITERATOR_TYPE_PRIVATE_DATA_2A
    if (PRIVATE_DATA(cc))
      {
      private_count = 2;
      private_srcw[0] = PRIVATE_DATA(cc);
      private_srcw[1] = private_srcw[0] + sizeof(sljit_sw);
      }
    cc += 1;
    break;

    CASE_ITERATOR_TYPE_PRIVATE_DATA_2B
    if (PRIVATE_DATA(cc))
      {
      private_count = 2;
      private_srcw[0] = PRIVATE_DATA(cc);
      private_srcw[1] = private_srcw[0] + sizeof(sljit_sw);
      }
    cc += 1 + IMM2_SIZE;
    break;

    case OP_CLASS:
    case OP_NCLASS:
#if defined SUPPORT_UNICODE || PCRE2_CODE_UNIT_WIDTH != 8
    case OP_XCLASS:
    i = (*cc == OP_XCLASS) ? GET(cc, 1) : 1 + 32 / (int)sizeof(PCRE2_UCHAR);
#else
    i = 1 + 32 / (int)sizeof(PCRE2_UCHAR);
#endif
    if (PRIVATE_DATA(cc) != 0)
      switch(get_class_iterator_size(cc + i))
        {
        case 1:
        private_count = 1;
        private_srcw[0] = PRIVATE_DATA(cc);
        break;

        case 2:
        private_count = 2;
        private_srcw[0] = PRIVATE_DATA(cc);
        private_srcw[1] = private_srcw[0] + sizeof(sljit_sw);
        break;

        default:
        SLJIT_UNREACHABLE();
        break;
        }
    cc += i;
    break;

    case OP_MARK:
    case OP_COMMIT_ARG:
    case OP_PRUNE_ARG:
    case OP_THEN_ARG:
    SLJIT_ASSERT(common->mark_ptr != 0);
    if (has_quit && !setmark_found)
      {
      kept_shared_srcw[0] = common->mark_ptr;
      kept_shared_count = 1;
      setmark_found = TRUE;
      }
    if (common->control_head_ptr != 0 && !control_head_found)
      {
      private_srcw[0] = common->control_head_ptr;
      private_count = 1;
      control_head_found = TRUE;
      }
    cc += 1 + 2 + cc[1];
    break;

    case OP_THEN:
    SLJIT_ASSERT(common->control_head_ptr != 0);
    if (!control_head_found)
      {
      private_srcw[0] = common->control_head_ptr;
      private_count = 1;
      control_head_found = TRUE;
      }
    cc++;
    break;

    default:
    cc = next_opcode(common, cc);
    SLJIT_ASSERT(cc != NULL);
    break;
    }

  if (type != recurse_copy_shared_to_global && type != recurse_copy_kept_shared_to_global)
    {
    SLJIT_ASSERT(type == recurse_copy_from_global || type == recurse_copy_private_to_global || type == recurse_swap_global);

    for (i = 0; i < private_count; i++)
      {
      SLJIT_ASSERT(private_srcw[i] != 0);

      if (!from_sp)
        delayed_mem_copy_move(&status, base_reg, stackptr, SLJIT_SP, private_srcw[i]);

      if (from_sp || type == recurse_swap_global)
        delayed_mem_copy_move(&status, SLJIT_SP, private_srcw[i], base_reg, stackptr);

      stackptr += sizeof(sljit_sw);
      }
    }
  else
    stackptr += sizeof(sljit_sw) * private_count;

  if (type != recurse_copy_private_to_global && type != recurse_copy_kept_shared_to_global)
    {
    SLJIT_ASSERT(type == recurse_copy_from_global || type == recurse_copy_shared_to_global || type == recurse_swap_global);

    for (i = 0; i < shared_count; i++)
      {
      SLJIT_ASSERT(shared_srcw[i] != 0);

      if (!from_sp)
        delayed_mem_copy_move(&status, base_reg, stackptr, SLJIT_SP, shared_srcw[i]);

      if (from_sp || type == recurse_swap_global)
        delayed_mem_copy_move(&status, SLJIT_SP, shared_srcw[i], base_reg, stackptr);

      stackptr += sizeof(sljit_sw);
      }
    }
  else
    stackptr += sizeof(sljit_sw) * shared_count;

  if (type != recurse_copy_private_to_global && type != recurse_swap_global)
    {
    SLJIT_ASSERT(type == recurse_copy_from_global || type == recurse_copy_shared_to_global || type == recurse_copy_kept_shared_to_global);

    for (i = 0; i < kept_shared_count; i++)
      {
      SLJIT_ASSERT(kept_shared_srcw[i] != 0);

      if (!from_sp)
        delayed_mem_copy_move(&status, base_reg, stackptr, SLJIT_SP, kept_shared_srcw[i]);

      if (from_sp || type == recurse_swap_global)
        delayed_mem_copy_move(&status, SLJIT_SP, kept_shared_srcw[i], base_reg, stackptr);

      stackptr += sizeof(sljit_sw);
      }
    }
  else
    stackptr += sizeof(sljit_sw) * kept_shared_count;
  }

SLJIT_ASSERT(cc == ccend && stackptr == stacktop);

delayed_mem_copy_finish(&status);
}