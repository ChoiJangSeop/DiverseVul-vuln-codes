int yr_re_fast_exec(
    uint8_t* code,
    uint8_t* input_data,
    size_t input_size,
    int flags,
    RE_MATCH_CALLBACK_FUNC callback,
    void* callback_args)
{
  RE_REPEAT_ANY_ARGS* repeat_any_args;

  uint8_t* code_stack[MAX_FAST_RE_STACK];
  uint8_t* input_stack[MAX_FAST_RE_STACK];
  int matches_stack[MAX_FAST_RE_STACK];

  uint8_t* ip = code;
  uint8_t* input = input_data;
  uint8_t* next_input;
  uint8_t* next_opcode;
  uint8_t mask;
  uint8_t value;

  int i;
  int stop;
  int input_incr;
  int sp = 0;
  int bytes_matched;
  int max_bytes_matched = input_size;

  input_incr = flags & RE_FLAGS_BACKWARDS ? -1 : 1;

  if (flags & RE_FLAGS_BACKWARDS)
    input--;

  code_stack[sp] = code;
  input_stack[sp] = input;
  matches_stack[sp] = 0;
  sp++;

  while (sp > 0)
  {
    sp--;
    ip = code_stack[sp];
    input = input_stack[sp];
    bytes_matched = matches_stack[sp];
    stop = FALSE;

    while(!stop)
    {
      if (*ip == RE_OPCODE_MATCH)
      {
        if (flags & RE_FLAGS_EXHAUSTIVE)
        {
          int cb_result = callback(
             flags & RE_FLAGS_BACKWARDS ? input + 1 : input_data,
             bytes_matched,
             flags,
             callback_args);

          switch(cb_result)
          {
            case ERROR_INSUFFICIENT_MEMORY:
              return -2;
            case ERROR_TOO_MANY_MATCHES:
              return -3;
            default:
              if (cb_result != ERROR_SUCCESS)
                return -4;
          }

          break;
        }
        else
        {
          return bytes_matched;
        }
      }

      if (bytes_matched >= max_bytes_matched)
        break;

      switch(*ip)
      {
        case RE_OPCODE_LITERAL:

          if (*input == *(ip + 1))
          {
            bytes_matched++;
            input += input_incr;
            ip += 2;
          }
          else
          {
            stop = TRUE;
          }

          break;

        case RE_OPCODE_MASKED_LITERAL:

          value = *(int16_t*)(ip + 1) & 0xFF;
          mask = *(int16_t*)(ip + 1) >> 8;

          if ((*input & mask) == value)
          {
            bytes_matched++;
            input += input_incr;
            ip += 3;
          }
          else
          {
            stop = TRUE;
          }

          break;

        case RE_OPCODE_ANY:

          bytes_matched++;
          input += input_incr;
          ip += 1;

          break;

        case RE_OPCODE_REPEAT_ANY_UNGREEDY:

          repeat_any_args = (RE_REPEAT_ANY_ARGS*)(ip + 1);
          next_opcode = ip + 1 + sizeof(RE_REPEAT_ANY_ARGS);

          for (i = repeat_any_args->min + 1; i <= repeat_any_args->max; i++)
          {
            next_input = input + i * input_incr;

            if (bytes_matched + i >= max_bytes_matched)
              break;

            if ( *(next_opcode) != RE_OPCODE_LITERAL ||
                (*(next_opcode) == RE_OPCODE_LITERAL &&
                 *(next_opcode + 1) == *next_input))
            {
              if (sp >= MAX_FAST_RE_STACK)
                return -4;

              code_stack[sp] = next_opcode;
              input_stack[sp] = next_input;
              matches_stack[sp] = bytes_matched + i;
              sp++;
            }
          }

          input += input_incr * repeat_any_args->min;
          bytes_matched += repeat_any_args->min;
          ip = next_opcode;

          break;

        default:
          assert(FALSE);
      }
    }
  }

  return -1;
}