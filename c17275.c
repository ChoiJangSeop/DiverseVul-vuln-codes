static bool check_vcol_forward_refs(Field *field, Virtual_column_info *vcol,
                                    bool check_constraint)
{
  bool res;
  uint32 flags= field->flags;
  if (check_constraint)
  {
    /* Check constraints can refer it itself */
    field->flags|= NO_DEFAULT_VALUE_FLAG;
  }
  res= (vcol &&
        vcol->expr->walk(&Item::check_field_expression_processor, 0, field));
  field->flags= flags;
  return res;
}