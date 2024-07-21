bson_iter_as_double (const bson_iter_t *iter) /* IN */
{
  BSON_ASSERT (iter);

  switch ((int) ITER_TYPE (iter)) {
  case BSON_TYPE_BOOL:
    return (double) bson_iter_bool (iter);
  case BSON_TYPE_DOUBLE:
    return bson_iter_double (iter);
  case BSON_TYPE_INT32:
    return (double) bson_iter_int32 (iter);
  case BSON_TYPE_INT64:
    return (double) bson_iter_int64 (iter);
  default:
    return 0;
  }
}