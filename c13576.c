static void SerializeGltfSampler(Sampler &sampler, json &o) {
  if (sampler.magFilter != -1) {
    SerializeNumberProperty("magFilter", sampler.magFilter, o);
  }
  if (sampler.minFilter != -1) {
    SerializeNumberProperty("minFilter", sampler.minFilter, o);
  }
  //SerializeNumberProperty("wrapR", sampler.wrapR, o);
  SerializeNumberProperty("wrapS", sampler.wrapS, o);
  SerializeNumberProperty("wrapT", sampler.wrapT, o);

  if (sampler.extras.Type() != NULL_TYPE) {
    SerializeValue("extras", sampler.extras, o);
  }
}