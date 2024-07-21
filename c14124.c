hash_new_from_values(mrb_state *mrb, mrb_int argc, mrb_value *regs)
{
  mrb_value hash = mrb_hash_new_capa(mrb, argc);
  while (argc--) {
    mrb_hash_set(mrb, hash, regs[0], regs[1]);
    regs += 2;
  }
  return hash;
}