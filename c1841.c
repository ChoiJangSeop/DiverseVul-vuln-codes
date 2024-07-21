void HGraph::EliminateRedundantBoundsChecks(HBasicBlock* bb,
                                            BoundsCheckTable* table) {
  BoundsCheckBbData* bb_data_list = NULL;

  for (HInstruction* i = bb->first(); i != NULL; i = i->next()) {
    if (!i->IsBoundsCheck()) continue;

    HBoundsCheck* check = HBoundsCheck::cast(i);
    check->ReplaceAllUsesWith(check->index());

    if (!FLAG_array_bounds_checks_elimination) continue;

    int32_t offset;
    BoundsCheckKey* key =
        BoundsCheckKey::Create(zone(), check, &offset);
    if (key == NULL) continue;
    BoundsCheckBbData** data_p = table->LookupOrInsert(key, zone());
    BoundsCheckBbData* data = *data_p;
    if (data == NULL) {
      bb_data_list = new(zone()) BoundsCheckBbData(key,
                                                   offset,
                                                   offset,
                                                   bb,
                                                   check,
                                                   check,
                                                   bb_data_list,
                                                   NULL);
      *data_p = bb_data_list;
    } else if (data->OffsetIsCovered(offset)) {
      check->DeleteAndReplaceWith(NULL);
    } else if (data->BasicBlock() == bb) {
      data->CoverCheck(check, offset);
    } else {
      int32_t new_lower_offset = offset < data->LowerOffset()
          ? offset
          : data->LowerOffset();
      int32_t new_upper_offset = offset > data->UpperOffset()
          ? offset
          : data->UpperOffset();
      bb_data_list = new(zone()) BoundsCheckBbData(key,
                                                   new_lower_offset,
                                                   new_upper_offset,
                                                   bb,
                                                   data->LowerCheck(),
                                                   data->UpperCheck(),
                                                   bb_data_list,
                                                   data);
      table->Insert(key, bb_data_list, zone());
    }
  }

  for (int i = 0; i < bb->dominated_blocks()->length(); ++i) {
    EliminateRedundantBoundsChecks(bb->dominated_blocks()->at(i), table);
  }

  for (BoundsCheckBbData* data = bb_data_list;
       data != NULL;
       data = data->NextInBasicBlock()) {
    data->RemoveZeroOperations();
    if (data->FatherInDominatorTree()) {
      table->Insert(data->Key(), data->FatherInDominatorTree(), zone());
    } else {
      table->Delete(data->Key());
    }
  }
}