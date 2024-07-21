bool Sampler::operator==(const Sampler &other) const {
  return this->extensions == other.extensions && this->extras == other.extras &&
         this->magFilter == other.magFilter &&
         this->minFilter == other.minFilter && this->name == other.name &&
         this->wrapS == other.wrapS &&
         this->wrapT == other.wrapT;

         //this->wrapR == other.wrapR
}