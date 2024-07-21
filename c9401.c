KEY_Reload(void)
{
  unsigned int i, line_number, key_length, cmac_key_length;
  FILE *in;
  char line[2048], *key_file, *key_value;
  const char *key_type;
  int hash_id;
  Key key;

  free_keys();

  key_file = CNF_GetKeysFile();
  line_number = 0;

  if (!key_file)
    return;

  in = fopen(key_file, "r");
  if (!in) {
    LOG(LOGS_WARN, "Could not open keyfile %s", key_file);
    return;
  }

  while (fgets(line, sizeof (line), in)) {
    line_number++;

    CPS_NormalizeLine(line);
    if (!*line)
      continue;

    memset(&key, 0, sizeof (key));

    if (!CPS_ParseKey(line, &key.id, &key_type, &key_value)) {
      LOG(LOGS_WARN, "Could not parse key at line %u in file %s", line_number, key_file);
      continue;
    }

    key_length = decode_key(key_value);
    if (key_length == 0) {
      LOG(LOGS_WARN, "Could not decode key %"PRIu32, key.id);
      continue;
    }

    hash_id = HSH_GetHashId(key_type);
    cmac_key_length = CMC_GetKeyLength(key_type);

    if (hash_id >= 0) {
      key.class = NTP_MAC;
      key.data.ntp_mac.value = MallocArray(unsigned char, key_length);
      memcpy(key.data.ntp_mac.value, key_value, key_length);
      key.data.ntp_mac.length = key_length;
      key.data.ntp_mac.hash_id = hash_id;
    } else if (cmac_key_length > 0) {
      if (cmac_key_length != key_length) {
        LOG(LOGS_WARN, "Invalid length of %s key %"PRIu32" (expected %u bits)",
            key_type, key.id, 8 * cmac_key_length);
        continue;
      }

      key.class = CMAC;
      key.data.cmac = CMC_CreateInstance(key_type, (unsigned char *)key_value, key_length);
      assert(key.data.cmac);
    } else {
      LOG(LOGS_WARN, "Unknown hash function or cipher in key %"PRIu32, key.id);
      continue;
    }

    ARR_AppendElement(keys, &key);
  }

  fclose(in);

  /* Sort keys into order.  Note, if there's a duplicate, it is
     arbitrary which one we use later - the user should have been
     more careful! */
  qsort(ARR_GetElements(keys), ARR_GetSize(keys), sizeof (Key), compare_keys_by_id);

  /* Check for duplicates */
  for (i = 1; i < ARR_GetSize(keys); i++) {
    if (get_key(i - 1)->id == get_key(i)->id)
      LOG(LOGS_WARN, "Detected duplicate key %"PRIu32, get_key(i - 1)->id);
  }

  /* Erase any passwords from stack */
  memset(line, 0, sizeof (line));

  for (i = 0; i < ARR_GetSize(keys); i++)
    get_key(i)->auth_delay = determine_hash_delay(get_key(i)->id);
}