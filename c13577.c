static bool ParseSampler(Sampler *sampler, std::string *err, const json &o,
                         bool store_original_json_for_extras_and_extensions) {
  ParseStringProperty(&sampler->name, err, o, "name", false);

  int minFilter = -1;
  int magFilter = -1;
  int wrapS = TINYGLTF_TEXTURE_WRAP_REPEAT;
  int wrapT = TINYGLTF_TEXTURE_WRAP_REPEAT;
  //int wrapR = TINYGLTF_TEXTURE_WRAP_REPEAT;
  ParseIntegerProperty(&minFilter, err, o, "minFilter", false);
  ParseIntegerProperty(&magFilter, err, o, "magFilter", false);
  ParseIntegerProperty(&wrapS, err, o, "wrapS", false);
  ParseIntegerProperty(&wrapT, err, o, "wrapT", false);
  //ParseIntegerProperty(&wrapR, err, o, "wrapR", false);  // tinygltf extension

  // TODO(syoyo): Check the value is alloed one.
  // (e.g. we allow 9728(NEAREST), but don't allow 9727)

  sampler->minFilter = minFilter;
  sampler->magFilter = magFilter;
  sampler->wrapS = wrapS;
  sampler->wrapT = wrapT;
  //sampler->wrapR = wrapR;

  ParseExtensionsProperty(&(sampler->extensions), err, o);
  ParseExtrasProperty(&(sampler->extras), o);

  if (store_original_json_for_extras_and_extensions) {
    {
      json_const_iterator it;
      if (FindMember(o, "extensions", it)) {
        sampler->extensions_json_string = JsonToString(GetValue(it));
      }
    }
    {
      json_const_iterator it;
      if (FindMember(o, "extras", it)) {
        sampler->extras_json_string = JsonToString(GetValue(it));
      }
    }
  }

  return true;
}