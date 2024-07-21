static int jit_compile(pcre2_code *code, sljit_u32 mode)
{
pcre2_real_code *re = (pcre2_real_code *)code;
struct sljit_compiler *compiler;
backtrack_common rootbacktrack;
compiler_common common_data;
compiler_common *common = &common_data;
const sljit_u8 *tables = re->tables;
void *allocator_data = &re->memctl;
int private_data_size;
PCRE2_SPTR ccend;
executable_functions *functions;
void *executable_func;
sljit_uw executable_size;
sljit_uw total_length;
struct sljit_label *mainloop_label = NULL;
struct sljit_label *continue_match_label;
struct sljit_label *empty_match_found_label = NULL;
struct sljit_label *empty_match_backtrack_label = NULL;
struct sljit_label *reset_match_label;
struct sljit_label *quit_label;
struct sljit_jump *jump;
struct sljit_jump *minlength_check_failed = NULL;
struct sljit_jump *empty_match = NULL;
struct sljit_jump *end_anchor_failed = NULL;
jump_list *reqcu_not_found = NULL;

SLJIT_ASSERT(tables);

#if HAS_VIRTUAL_REGISTERS == 1
SLJIT_ASSERT(sljit_get_register_index(TMP3) < 0 && sljit_get_register_index(ARGUMENTS) < 0 && sljit_get_register_index(RETURN_ADDR) < 0);
#elif HAS_VIRTUAL_REGISTERS == 0
SLJIT_ASSERT(sljit_get_register_index(TMP3) >= 0 && sljit_get_register_index(ARGUMENTS) >= 0 && sljit_get_register_index(RETURN_ADDR) >= 0);
#else
#error "Invalid value for HAS_VIRTUAL_REGISTERS"
#endif

memset(&rootbacktrack, 0, sizeof(backtrack_common));
memset(common, 0, sizeof(compiler_common));
common->re = re;
common->name_table = (PCRE2_SPTR)((uint8_t *)re + sizeof(pcre2_real_code));
rootbacktrack.cc = common->name_table + re->name_count * re->name_entry_size;

#ifdef SUPPORT_UNICODE
common->invalid_utf = (mode & PCRE2_JIT_INVALID_UTF) != 0;
#endif /* SUPPORT_UNICODE */
mode &= ~PUBLIC_JIT_COMPILE_CONFIGURATION_OPTIONS;

common->start = rootbacktrack.cc;
common->read_only_data_head = NULL;
common->fcc = tables + fcc_offset;
common->lcc = (sljit_sw)(tables + lcc_offset);
common->mode = mode;
common->might_be_empty = (re->minlength == 0) || (re->flags & PCRE2_MATCH_EMPTY);
common->allow_empty_partial = (re->max_lookbehind > 0) || (re->flags & PCRE2_MATCH_EMPTY);
common->nltype = NLTYPE_FIXED;
switch(re->newline_convention)
  {
  case PCRE2_NEWLINE_CR: common->newline = CHAR_CR; break;
  case PCRE2_NEWLINE_LF: common->newline = CHAR_NL; break;
  case PCRE2_NEWLINE_CRLF: common->newline = (CHAR_CR << 8) | CHAR_NL; break;
  case PCRE2_NEWLINE_ANY: common->newline = (CHAR_CR << 8) | CHAR_NL; common->nltype = NLTYPE_ANY; break;
  case PCRE2_NEWLINE_ANYCRLF: common->newline = (CHAR_CR << 8) | CHAR_NL; common->nltype = NLTYPE_ANYCRLF; break;
  case PCRE2_NEWLINE_NUL: common->newline = CHAR_NUL; break;
  default: return PCRE2_ERROR_INTERNAL;
  }
common->nlmax = READ_CHAR_MAX;
common->nlmin = 0;
if (re->bsr_convention == PCRE2_BSR_UNICODE)
  common->bsr_nltype = NLTYPE_ANY;
else if (re->bsr_convention == PCRE2_BSR_ANYCRLF)
  common->bsr_nltype = NLTYPE_ANYCRLF;
else
  {
#ifdef BSR_ANYCRLF
  common->bsr_nltype = NLTYPE_ANYCRLF;
#else
  common->bsr_nltype = NLTYPE_ANY;
#endif
  }
common->bsr_nlmax = READ_CHAR_MAX;
common->bsr_nlmin = 0;
common->endonly = (re->overall_options & PCRE2_DOLLAR_ENDONLY) != 0;
common->ctypes = (sljit_sw)(tables + ctypes_offset);
common->name_count = re->name_count;
common->name_entry_size = re->name_entry_size;
common->unset_backref = (re->overall_options & PCRE2_MATCH_UNSET_BACKREF) != 0;
common->alt_circumflex = (re->overall_options & PCRE2_ALT_CIRCUMFLEX) != 0;
#ifdef SUPPORT_UNICODE
/* PCRE_UTF[16|32] have the same value as PCRE_UTF8. */
common->utf = (re->overall_options & PCRE2_UTF) != 0;
common->ucp = (re->overall_options & PCRE2_UCP) != 0;
if (common->utf)
  {
  if (common->nltype == NLTYPE_ANY)
    common->nlmax = 0x2029;
  else if (common->nltype == NLTYPE_ANYCRLF)
    common->nlmax = (CHAR_CR > CHAR_NL) ? CHAR_CR : CHAR_NL;
  else
    {
    /* We only care about the first newline character. */
    common->nlmax = common->newline & 0xff;
    }

  if (common->nltype == NLTYPE_FIXED)
    common->nlmin = common->newline & 0xff;
  else
    common->nlmin = (CHAR_CR < CHAR_NL) ? CHAR_CR : CHAR_NL;

  if (common->bsr_nltype == NLTYPE_ANY)
    common->bsr_nlmax = 0x2029;
  else
    common->bsr_nlmax = (CHAR_CR > CHAR_NL) ? CHAR_CR : CHAR_NL;
  common->bsr_nlmin = (CHAR_CR < CHAR_NL) ? CHAR_CR : CHAR_NL;
  }
else
  common->invalid_utf = FALSE;
#endif /* SUPPORT_UNICODE */
ccend = bracketend(common->start);

/* Calculate the local space size on the stack. */
common->ovector_start = LIMIT_MATCH + sizeof(sljit_sw);
common->optimized_cbracket = (sljit_u8 *)SLJIT_MALLOC(re->top_bracket + 1, allocator_data);
if (!common->optimized_cbracket)
  return PCRE2_ERROR_NOMEMORY;
#if defined DEBUG_FORCE_UNOPTIMIZED_CBRAS && DEBUG_FORCE_UNOPTIMIZED_CBRAS == 1
memset(common->optimized_cbracket, 0, re->top_bracket + 1);
#else
memset(common->optimized_cbracket, 1, re->top_bracket + 1);
#endif

SLJIT_ASSERT(*common->start == OP_BRA && ccend[-(1 + LINK_SIZE)] == OP_KET);
#if defined DEBUG_FORCE_UNOPTIMIZED_CBRAS && DEBUG_FORCE_UNOPTIMIZED_CBRAS == 2
common->capture_last_ptr = common->ovector_start;
common->ovector_start += sizeof(sljit_sw);
#endif
if (!check_opcode_types(common, common->start, ccend))
  {
  SLJIT_FREE(common->optimized_cbracket, allocator_data);
  return PCRE2_ERROR_NOMEMORY;
  }

/* Checking flags and updating ovector_start. */
if (mode == PCRE2_JIT_COMPLETE && (re->flags & PCRE2_LASTSET) != 0 && (re->overall_options & PCRE2_NO_START_OPTIMIZE) == 0)
  {
  common->req_char_ptr = common->ovector_start;
  common->ovector_start += sizeof(sljit_sw);
  }
if (mode != PCRE2_JIT_COMPLETE)
  {
  common->start_used_ptr = common->ovector_start;
  common->ovector_start += sizeof(sljit_sw);
  if (mode == PCRE2_JIT_PARTIAL_SOFT)
    {
    common->hit_start = common->ovector_start;
    common->ovector_start += sizeof(sljit_sw);
    }
  }
if ((re->overall_options & (PCRE2_FIRSTLINE | PCRE2_USE_OFFSET_LIMIT)) != 0)
  {
  common->match_end_ptr = common->ovector_start;
  common->ovector_start += sizeof(sljit_sw);
  }
#if defined DEBUG_FORCE_CONTROL_HEAD && DEBUG_FORCE_CONTROL_HEAD
common->control_head_ptr = 1;
#endif
if (common->control_head_ptr != 0)
  {
  common->control_head_ptr = common->ovector_start;
  common->ovector_start += sizeof(sljit_sw);
  }
if (common->has_set_som)
  {
  /* Saving the real start pointer is necessary. */
  common->start_ptr = common->ovector_start;
  common->ovector_start += sizeof(sljit_sw);
  }

/* Aligning ovector to even number of sljit words. */
if ((common->ovector_start & sizeof(sljit_sw)) != 0)
  common->ovector_start += sizeof(sljit_sw);

if (common->start_ptr == 0)
  common->start_ptr = OVECTOR(0);

/* Capturing brackets cannot be optimized if callouts are allowed. */
if (common->capture_last_ptr != 0)
  memset(common->optimized_cbracket, 0, re->top_bracket + 1);

SLJIT_ASSERT(!(common->req_char_ptr != 0 && common->start_used_ptr != 0));
common->cbra_ptr = OVECTOR_START + (re->top_bracket + 1) * 2 * sizeof(sljit_sw);

total_length = ccend - common->start;
common->private_data_ptrs = (sljit_s32 *)SLJIT_MALLOC(total_length * (sizeof(sljit_s32) + (common->has_then ? 1 : 0)), allocator_data);
if (!common->private_data_ptrs)
  {
  SLJIT_FREE(common->optimized_cbracket, allocator_data);
  return PCRE2_ERROR_NOMEMORY;
  }
memset(common->private_data_ptrs, 0, total_length * sizeof(sljit_s32));

private_data_size = common->cbra_ptr + (re->top_bracket + 1) * sizeof(sljit_sw);

if ((re->overall_options & PCRE2_ANCHORED) == 0 && (re->overall_options & PCRE2_NO_START_OPTIMIZE) == 0 && !common->has_skip_in_assert_back)
  detect_early_fail(common, common->start, &private_data_size, 0, 0, TRUE);

set_private_data_ptrs(common, &private_data_size, ccend);

SLJIT_ASSERT(common->early_fail_start_ptr <= common->early_fail_end_ptr);

if (private_data_size > SLJIT_MAX_LOCAL_SIZE)
  {
  SLJIT_FREE(common->private_data_ptrs, allocator_data);
  SLJIT_FREE(common->optimized_cbracket, allocator_data);
  return PCRE2_ERROR_NOMEMORY;
  }

if (common->has_then)
  {
  common->then_offsets = (sljit_u8 *)(common->private_data_ptrs + total_length);
  memset(common->then_offsets, 0, total_length);
  set_then_offsets(common, common->start, NULL);
  }

compiler = sljit_create_compiler(allocator_data, NULL);
if (!compiler)
  {
  SLJIT_FREE(common->optimized_cbracket, allocator_data);
  SLJIT_FREE(common->private_data_ptrs, allocator_data);
  return PCRE2_ERROR_NOMEMORY;
  }
common->compiler = compiler;

/* Main pcre2_jit_exec entry. */
sljit_emit_enter(compiler, 0, SLJIT_ARGS1(W, W), 5, 5, 0, 0, private_data_size);

/* Register init. */
reset_ovector(common, (re->top_bracket + 1) * 2);
if (common->req_char_ptr != 0)
  OP1(SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), common->req_char_ptr, SLJIT_R0, 0);

OP1(SLJIT_MOV, ARGUMENTS, 0, SLJIT_S0, 0);
OP1(SLJIT_MOV, TMP1, 0, SLJIT_S0, 0);
OP1(SLJIT_MOV, STR_PTR, 0, SLJIT_MEM1(TMP1), SLJIT_OFFSETOF(jit_arguments, str));
OP1(SLJIT_MOV, STR_END, 0, SLJIT_MEM1(TMP1), SLJIT_OFFSETOF(jit_arguments, end));
OP1(SLJIT_MOV, TMP2, 0, SLJIT_MEM1(TMP1), SLJIT_OFFSETOF(jit_arguments, stack));
OP1(SLJIT_MOV_U32, TMP1, 0, SLJIT_MEM1(TMP1), SLJIT_OFFSETOF(jit_arguments, limit_match));
OP1(SLJIT_MOV, STACK_TOP, 0, SLJIT_MEM1(TMP2), SLJIT_OFFSETOF(struct sljit_stack, end));
OP1(SLJIT_MOV, STACK_LIMIT, 0, SLJIT_MEM1(TMP2), SLJIT_OFFSETOF(struct sljit_stack, start));
OP2(SLJIT_ADD, TMP1, 0, TMP1, 0, SLJIT_IMM, 1);
OP1(SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), LIMIT_MATCH, TMP1, 0);

if (common->early_fail_start_ptr < common->early_fail_end_ptr)
  reset_early_fail(common);

if (mode == PCRE2_JIT_PARTIAL_SOFT)
  OP1(SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), common->hit_start, SLJIT_IMM, -1);
if (common->mark_ptr != 0)
  OP1(SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), common->mark_ptr, SLJIT_IMM, 0);
if (common->control_head_ptr != 0)
  OP1(SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), common->control_head_ptr, SLJIT_IMM, 0);

/* Main part of the matching */
if ((re->overall_options & PCRE2_ANCHORED) == 0)
  {
  mainloop_label = mainloop_entry(common);
  continue_match_label = LABEL();
  /* Forward search if possible. */
  if ((re->overall_options & PCRE2_NO_START_OPTIMIZE) == 0)
    {
    if (mode == PCRE2_JIT_COMPLETE && fast_forward_first_n_chars(common))
      ;
    else if ((re->flags & PCRE2_FIRSTSET) != 0)
      fast_forward_first_char(common);
    else if ((re->flags & PCRE2_STARTLINE) != 0)
      fast_forward_newline(common);
    else if ((re->flags & PCRE2_FIRSTMAPSET) != 0)
      fast_forward_start_bits(common);
    }
  }
else
  continue_match_label = LABEL();

if (mode == PCRE2_JIT_COMPLETE && re->minlength > 0 && (re->overall_options & PCRE2_NO_START_OPTIMIZE) == 0)
  {
  OP1(SLJIT_MOV, SLJIT_RETURN_REG, 0, SLJIT_IMM, PCRE2_ERROR_NOMATCH);
  OP2(SLJIT_ADD, TMP2, 0, STR_PTR, 0, SLJIT_IMM, IN_UCHARS(re->minlength));
  minlength_check_failed = CMP(SLJIT_GREATER, TMP2, 0, STR_END, 0);
  }
if (common->req_char_ptr != 0)
  reqcu_not_found = search_requested_char(common, (PCRE2_UCHAR)(re->last_codeunit), (re->flags & PCRE2_LASTCASELESS) != 0, (re->flags & PCRE2_FIRSTSET) != 0);

/* Store the current STR_PTR in OVECTOR(0). */
OP1(SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), OVECTOR(0), STR_PTR, 0);
/* Copy the limit of allowed recursions. */
OP1(SLJIT_MOV, COUNT_MATCH, 0, SLJIT_MEM1(SLJIT_SP), LIMIT_MATCH);
if (common->capture_last_ptr != 0)
  OP1(SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), common->capture_last_ptr, SLJIT_IMM, 0);
if (common->fast_forward_bc_ptr != NULL)
  OP1(SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), PRIVATE_DATA(common->fast_forward_bc_ptr + 1) >> 3, STR_PTR, 0);

if (common->start_ptr != OVECTOR(0))
  OP1(SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), common->start_ptr, STR_PTR, 0);

/* Copy the beginning of the string. */
if (mode == PCRE2_JIT_PARTIAL_SOFT)
  {
  jump = CMP(SLJIT_NOT_EQUAL, SLJIT_MEM1(SLJIT_SP), common->hit_start, SLJIT_IMM, -1);
  OP1(SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), common->start_used_ptr, STR_PTR, 0);
  JUMPHERE(jump);
  }
else if (mode == PCRE2_JIT_PARTIAL_HARD)
  OP1(SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), common->start_used_ptr, STR_PTR, 0);

compile_matchingpath(common, common->start, ccend, &rootbacktrack);
if (SLJIT_UNLIKELY(sljit_get_compiler_error(compiler)))
  {
  sljit_free_compiler(compiler);
  SLJIT_FREE(common->optimized_cbracket, allocator_data);
  SLJIT_FREE(common->private_data_ptrs, allocator_data);
  PRIV(jit_free_rodata)(common->read_only_data_head, allocator_data);
  return PCRE2_ERROR_NOMEMORY;
  }

if ((re->overall_options & PCRE2_ENDANCHORED) != 0)
  end_anchor_failed = CMP(SLJIT_NOT_EQUAL, STR_PTR, 0, STR_END, 0);

if (common->might_be_empty)
  {
  empty_match = CMP(SLJIT_EQUAL, STR_PTR, 0, SLJIT_MEM1(SLJIT_SP), OVECTOR(0));
  empty_match_found_label = LABEL();
  }

common->accept_label = LABEL();
if (common->accept != NULL)
  set_jumps(common->accept, common->accept_label);

/* This means we have a match. Update the ovector. */
copy_ovector(common, re->top_bracket + 1);
common->quit_label = common->abort_label = LABEL();
if (common->quit != NULL)
  set_jumps(common->quit, common->quit_label);
if (common->abort != NULL)
  set_jumps(common->abort, common->abort_label);
if (minlength_check_failed != NULL)
  SET_LABEL(minlength_check_failed, common->abort_label);

sljit_emit_op0(compiler, SLJIT_SKIP_FRAMES_BEFORE_RETURN);
sljit_emit_return(compiler, SLJIT_MOV, SLJIT_RETURN_REG, 0);

if (common->failed_match != NULL)
  {
  SLJIT_ASSERT(common->mode == PCRE2_JIT_COMPLETE);
  set_jumps(common->failed_match, LABEL());
  OP1(SLJIT_MOV, SLJIT_RETURN_REG, 0, SLJIT_IMM, PCRE2_ERROR_NOMATCH);
  JUMPTO(SLJIT_JUMP, common->abort_label);
  }

if ((re->overall_options & PCRE2_ENDANCHORED) != 0)
  JUMPHERE(end_anchor_failed);

if (mode != PCRE2_JIT_COMPLETE)
  {
  common->partialmatchlabel = LABEL();
  set_jumps(common->partialmatch, common->partialmatchlabel);
  return_with_partial_match(common, common->quit_label);
  }

if (common->might_be_empty)
  empty_match_backtrack_label = LABEL();
compile_backtrackingpath(common, rootbacktrack.top);
if (SLJIT_UNLIKELY(sljit_get_compiler_error(compiler)))
  {
  sljit_free_compiler(compiler);
  SLJIT_FREE(common->optimized_cbracket, allocator_data);
  SLJIT_FREE(common->private_data_ptrs, allocator_data);
  PRIV(jit_free_rodata)(common->read_only_data_head, allocator_data);
  return PCRE2_ERROR_NOMEMORY;
  }

SLJIT_ASSERT(rootbacktrack.prev == NULL);
reset_match_label = LABEL();

if (mode == PCRE2_JIT_PARTIAL_SOFT)
  {
  /* Update hit_start only in the first time. */
  jump = CMP(SLJIT_NOT_EQUAL, SLJIT_MEM1(SLJIT_SP), common->hit_start, SLJIT_IMM, 0);
  OP1(SLJIT_MOV, TMP1, 0, SLJIT_MEM1(SLJIT_SP), common->start_ptr);
  OP1(SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), common->start_used_ptr, SLJIT_IMM, -1);
  OP1(SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), common->hit_start, TMP1, 0);
  JUMPHERE(jump);
  }

/* Check we have remaining characters. */
if ((re->overall_options & PCRE2_ANCHORED) == 0 && common->match_end_ptr != 0)
  {
  OP1(SLJIT_MOV, TMP1, 0, SLJIT_MEM1(SLJIT_SP), common->match_end_ptr);
  }

OP1(SLJIT_MOV, STR_PTR, 0, SLJIT_MEM1(SLJIT_SP),
    (common->fast_forward_bc_ptr != NULL) ? (PRIVATE_DATA(common->fast_forward_bc_ptr + 1) >> 3) : common->start_ptr);

if ((re->overall_options & PCRE2_ANCHORED) == 0)
  {
  if (common->ff_newline_shortcut != NULL)
    {
    /* There cannot be more newlines if PCRE2_FIRSTLINE is set. */
    if ((re->overall_options & PCRE2_FIRSTLINE) == 0)
      {
      if (common->match_end_ptr != 0)
        {
        OP1(SLJIT_MOV, TMP3, 0, STR_END, 0);
        OP1(SLJIT_MOV, STR_END, 0, TMP1, 0);
        CMPTO(SLJIT_LESS, STR_PTR, 0, TMP1, 0, common->ff_newline_shortcut);
        OP1(SLJIT_MOV, STR_END, 0, TMP3, 0);
        }
      else
        CMPTO(SLJIT_LESS, STR_PTR, 0, STR_END, 0, common->ff_newline_shortcut);
      }
    }
  else
    CMPTO(SLJIT_LESS, STR_PTR, 0, (common->match_end_ptr == 0) ? STR_END : TMP1, 0, mainloop_label);
  }

/* No more remaining characters. */
if (reqcu_not_found != NULL)
  set_jumps(reqcu_not_found, LABEL());

if (mode == PCRE2_JIT_PARTIAL_SOFT)
  CMPTO(SLJIT_NOT_EQUAL, SLJIT_MEM1(SLJIT_SP), common->hit_start, SLJIT_IMM, -1, common->partialmatchlabel);

OP1(SLJIT_MOV, SLJIT_RETURN_REG, 0, SLJIT_IMM, PCRE2_ERROR_NOMATCH);
JUMPTO(SLJIT_JUMP, common->quit_label);

flush_stubs(common);

if (common->might_be_empty)
  {
  JUMPHERE(empty_match);
  OP1(SLJIT_MOV, TMP1, 0, ARGUMENTS, 0);
  OP1(SLJIT_MOV_U32, TMP2, 0, SLJIT_MEM1(TMP1), SLJIT_OFFSETOF(jit_arguments, options));
  OP2U(SLJIT_AND | SLJIT_SET_Z, TMP2, 0, SLJIT_IMM, PCRE2_NOTEMPTY);
  JUMPTO(SLJIT_NOT_ZERO, empty_match_backtrack_label);
  OP2U(SLJIT_AND | SLJIT_SET_Z, TMP2, 0, SLJIT_IMM, PCRE2_NOTEMPTY_ATSTART);
  JUMPTO(SLJIT_ZERO, empty_match_found_label);
  OP1(SLJIT_MOV, TMP2, 0, SLJIT_MEM1(TMP1), SLJIT_OFFSETOF(jit_arguments, str));
  CMPTO(SLJIT_NOT_EQUAL, TMP2, 0, STR_PTR, 0, empty_match_found_label);
  JUMPTO(SLJIT_JUMP, empty_match_backtrack_label);
  }

common->fast_forward_bc_ptr = NULL;
common->early_fail_start_ptr = 0;
common->early_fail_end_ptr = 0;
common->currententry = common->entries;
common->local_quit_available = TRUE;
quit_label = common->quit_label;
while (common->currententry != NULL)
  {
  /* Might add new entries. */
  compile_recurse(common);
  if (SLJIT_UNLIKELY(sljit_get_compiler_error(compiler)))
    {
    sljit_free_compiler(compiler);
    SLJIT_FREE(common->optimized_cbracket, allocator_data);
    SLJIT_FREE(common->private_data_ptrs, allocator_data);
    PRIV(jit_free_rodata)(common->read_only_data_head, allocator_data);
    return PCRE2_ERROR_NOMEMORY;
    }
  flush_stubs(common);
  common->currententry = common->currententry->next;
  }
common->local_quit_available = FALSE;
common->quit_label = quit_label;

/* Allocating stack, returns with PCRE_ERROR_JIT_STACKLIMIT if fails. */
/* This is a (really) rare case. */
set_jumps(common->stackalloc, LABEL());
/* RETURN_ADDR is not a saved register. */
sljit_emit_fast_enter(compiler, SLJIT_MEM1(SLJIT_SP), LOCALS0);

SLJIT_ASSERT(TMP1 == SLJIT_R0 && STR_PTR == SLJIT_R1);

OP1(SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), LOCALS1, STR_PTR, 0);
OP1(SLJIT_MOV, SLJIT_R0, 0, ARGUMENTS, 0);
OP2(SLJIT_SUB, SLJIT_R1, 0, STACK_LIMIT, 0, SLJIT_IMM, STACK_GROWTH_RATE);
OP1(SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_R0), SLJIT_OFFSETOF(jit_arguments, stack));
OP1(SLJIT_MOV, STACK_LIMIT, 0, TMP2, 0);

sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2(W, W, W), SLJIT_IMM, SLJIT_FUNC_ADDR(sljit_stack_resize));

jump = CMP(SLJIT_EQUAL, SLJIT_RETURN_REG, 0, SLJIT_IMM, 0);
OP1(SLJIT_MOV, TMP2, 0, STACK_LIMIT, 0);
OP1(SLJIT_MOV, STACK_LIMIT, 0, SLJIT_RETURN_REG, 0);
OP1(SLJIT_MOV, TMP1, 0, SLJIT_MEM1(SLJIT_SP), LOCALS0);
OP1(SLJIT_MOV, STR_PTR, 0, SLJIT_MEM1(SLJIT_SP), LOCALS1);
OP_SRC(SLJIT_FAST_RETURN, TMP1, 0);

/* Allocation failed. */
JUMPHERE(jump);
/* We break the return address cache here, but this is a really rare case. */
OP1(SLJIT_MOV, SLJIT_RETURN_REG, 0, SLJIT_IMM, PCRE2_ERROR_JIT_STACKLIMIT);
JUMPTO(SLJIT_JUMP, common->quit_label);

/* Call limit reached. */
set_jumps(common->calllimit, LABEL());
OP1(SLJIT_MOV, SLJIT_RETURN_REG, 0, SLJIT_IMM, PCRE2_ERROR_MATCHLIMIT);
JUMPTO(SLJIT_JUMP, common->quit_label);

if (common->revertframes != NULL)
  {
  set_jumps(common->revertframes, LABEL());
  do_revertframes(common);
  }
if (common->wordboundary != NULL)
  {
  set_jumps(common->wordboundary, LABEL());
  check_wordboundary(common);
  }
if (common->anynewline != NULL)
  {
  set_jumps(common->anynewline, LABEL());
  check_anynewline(common);
  }
if (common->hspace != NULL)
  {
  set_jumps(common->hspace, LABEL());
  check_hspace(common);
  }
if (common->vspace != NULL)
  {
  set_jumps(common->vspace, LABEL());
  check_vspace(common);
  }
if (common->casefulcmp != NULL)
  {
  set_jumps(common->casefulcmp, LABEL());
  do_casefulcmp(common);
  }
if (common->caselesscmp != NULL)
  {
  set_jumps(common->caselesscmp, LABEL());
  do_caselesscmp(common);
  }
if (common->reset_match != NULL)
  {
  set_jumps(common->reset_match, LABEL());
  do_reset_match(common, (re->top_bracket + 1) * 2);
  CMPTO(SLJIT_GREATER, STR_PTR, 0, TMP1, 0, continue_match_label);
  OP1(SLJIT_MOV, STR_PTR, 0, TMP1, 0);
  JUMPTO(SLJIT_JUMP, reset_match_label);
  }
#ifdef SUPPORT_UNICODE
#if PCRE2_CODE_UNIT_WIDTH == 8
if (common->utfreadchar != NULL)
  {
  set_jumps(common->utfreadchar, LABEL());
  do_utfreadchar(common);
  }
if (common->utfreadtype8 != NULL)
  {
  set_jumps(common->utfreadtype8, LABEL());
  do_utfreadtype8(common);
  }
if (common->utfpeakcharback != NULL)
  {
  set_jumps(common->utfpeakcharback, LABEL());
  do_utfpeakcharback(common);
  }
#endif /* PCRE2_CODE_UNIT_WIDTH == 8 */
#if PCRE2_CODE_UNIT_WIDTH == 8 || PCRE2_CODE_UNIT_WIDTH == 16
if (common->utfreadchar_invalid != NULL)
  {
  set_jumps(common->utfreadchar_invalid, LABEL());
  do_utfreadchar_invalid(common);
  }
if (common->utfreadnewline_invalid != NULL)
  {
  set_jumps(common->utfreadnewline_invalid, LABEL());
  do_utfreadnewline_invalid(common);
  }
if (common->utfmoveback_invalid)
  {
  set_jumps(common->utfmoveback_invalid, LABEL());
  do_utfmoveback_invalid(common);
  }
if (common->utfpeakcharback_invalid)
  {
  set_jumps(common->utfpeakcharback_invalid, LABEL());
  do_utfpeakcharback_invalid(common);
  }
#endif /* PCRE2_CODE_UNIT_WIDTH == 8 || PCRE2_CODE_UNIT_WIDTH == 16 */
if (common->getucd != NULL)
  {
  set_jumps(common->getucd, LABEL());
  do_getucd(common);
  }
if (common->getucdtype != NULL)
  {
  set_jumps(common->getucdtype, LABEL());
  do_getucdtype(common);
  }
#endif /* SUPPORT_UNICODE */

SLJIT_FREE(common->optimized_cbracket, allocator_data);
SLJIT_FREE(common->private_data_ptrs, allocator_data);

executable_func = sljit_generate_code(compiler);
executable_size = sljit_get_generated_code_size(compiler);
sljit_free_compiler(compiler);

if (executable_func == NULL)
  {
  PRIV(jit_free_rodata)(common->read_only_data_head, allocator_data);
  return PCRE2_ERROR_NOMEMORY;
  }

/* Reuse the function descriptor if possible. */
if (re->executable_jit != NULL)
  functions = (executable_functions *)re->executable_jit;
else
  {
  functions = SLJIT_MALLOC(sizeof(executable_functions), allocator_data);
  if (functions == NULL)
    {
    /* This case is highly unlikely since we just recently
    freed a lot of memory. Not impossible though. */
    sljit_free_code(executable_func, NULL);
    PRIV(jit_free_rodata)(common->read_only_data_head, allocator_data);
    return PCRE2_ERROR_NOMEMORY;
    }
  memset(functions, 0, sizeof(executable_functions));
  functions->top_bracket = re->top_bracket + 1;
  functions->limit_match = re->limit_match;
  re->executable_jit = functions;
  }

/* Turn mode into an index. */
if (mode == PCRE2_JIT_COMPLETE)
  mode = 0;
else
  mode = (mode == PCRE2_JIT_PARTIAL_SOFT) ? 1 : 2;

SLJIT_ASSERT(mode < JIT_NUMBER_OF_COMPILE_MODES);
functions->executable_funcs[mode] = executable_func;
functions->read_only_data_heads[mode] = common->read_only_data_head;
functions->executable_sizes[mode] = executable_size;
return 0;
}