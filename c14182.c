bool SNC_io_parser<EW>::
read_volume(Volume_handle ch) {

  bool OK = true;
  int index;
  char cc;

  in >> index;
  OK = OK && test_string("{");

  in >> cc;
  while(isdigit(cc)) {
    in.putback(cc);
    in >> index;
    ch->shell_entry_objects().push_back(make_object(SFace_of[index]));
    in >> cc;
  }
  in >> ch->mark();

  return OK;
}