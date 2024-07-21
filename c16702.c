CAMLprim value caml_blit_string(value s1, value ofs1, value s2, value ofs2,
                                value n)
{
  memmove(&Byte(s2, Long_val(ofs2)), &Byte(s1, Long_val(ofs1)), Int_val(n));
  return Val_unit;
}