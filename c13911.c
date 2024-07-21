gen_values(codegen_scope *s, node *t, int val, int limit)
{
  int n = 0;
  int first = 1;
  int slimit = GEN_VAL_STACK_MAX;

  if (limit == 0) limit = GEN_LIT_ARY_MAX;
  if (cursp() >= slimit) slimit = INT16_MAX;

  if (!val) {
    while (t) {
      codegen(s, t->car, NOVAL);
      n++;
      t = t->cdr;
    }
    return n;
  }

  while (t) {
    int is_splat = nint(t->car->car) == NODE_SPLAT;

    if (is_splat || n > limit || cursp() >= slimit) { /* flush stack */
      pop_n(n);
      if (first) {
        if (n == 0) {
          genop_1(s, OP_LOADNIL, cursp());
        }
        else {
          genop_2(s, OP_ARRAY, cursp(), n);
        }
        push();
        first = 0;
        limit = GEN_LIT_ARY_MAX;
      }
      else if (n > 0) {
        pop();
        genop_2(s, OP_ARYPUSH, cursp(), n);
        push();
      }
      n = 0;
    }
    codegen(s, t->car, val);
    if (is_splat) {
      pop(); pop();
      genop_1(s, OP_ARYCAT, cursp());
      push();
    }
    else {
      n++;
    }
    t = t->cdr;
  }
  if (!first) {
    pop();
    if (n > 0) {
      pop_n(n);
      genop_2(s, OP_ARYPUSH, cursp(), n);
    }
    return -1;                  /* variable length */
  }
  return n;
}