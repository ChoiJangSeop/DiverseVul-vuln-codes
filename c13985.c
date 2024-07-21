bool define_fastly_sys(JSContext *cx, HandleObject global) {
  // Allocating the reusable hostcall buffer here means it's baked into the
  // snapshot, and since it's all zeros, it won't increase the size of the
  // snapshot.
  if (!OwnedHostCallBuffer::initialize(cx))
    return false;

  if (!GlobalProperties::init(cx, global))
    return false;

  if (!builtins::Fastly::create(cx, global))
    return false;
  if (!builtins::Console::create(cx, global))
    return false;
  if (!builtins::Crypto::create(cx, global))
    return false;

  if (!builtins::NativeStreamSource::init_class(cx, global))
    return false;
  if (!builtins::NativeStreamSink::init_class(cx, global))
    return false;
  if (!builtins::TransformStreamDefaultController::init_class(cx, global))
    return false;
  if (!builtins::TransformStream::init_class(cx, global))
    return false;
  if (!CompressionStream::init_class(cx, global))
    return false;
  if (!DecompressionStream::init_class(cx, global))
    return false;
  if (!Request::init_class(cx, global))
    return false;
  if (!Response::init_class(cx, global))
    return false;
  if (!builtins::Dictionary::init_class(cx, global))
    return false;
  if (!Headers::init_class(cx, global))
    return false;
  if (!ClientInfo::init_class(cx, global))
    return false;
  if (!FetchEvent::init_class(cx, global))
    return false;
  if (!builtins::CacheOverride::init_class(cx, global))
    return false;
  if (!TextEncoder::init_class(cx, global))
    return false;
  if (!TextDecoder::init_class(cx, global))
    return false;
  if (!builtins::Logger::init_class(cx, global))
    return false;
  if (!URL::init_class(cx, global))
    return false;
  if (!URLSearchParams::init_class(cx, global))
    return false;
  if (!URLSearchParamsIterator::init_class(cx, global))
    return false;
  if (!builtins::WorkerLocation::init_class(cx, global))
    return false;
  if (!ObjectStore::init_class(cx, global))
    return false;
  if (!ObjectStoreEntry::init_class(cx, global))
    return false;

  pending_requests = new JS::PersistentRootedObjectVector(cx);
  pending_body_reads = new JS::PersistentRootedObjectVector(cx);

  return true;
}