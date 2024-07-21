v8::Local<v8::Object> CreateNativeEvent(
    v8::Isolate* isolate,
    v8::Local<v8::Object> sender,
    content::RenderFrameHost* frame,
    electron::mojom::ElectronBrowser::MessageSyncCallback callback) {
  v8::Local<v8::Object> event;
  if (frame && callback) {
    gin::Handle<Event> native_event = Event::Create(isolate);
    native_event->SetCallback(std::move(callback));
    event = v8::Local<v8::Object>::Cast(native_event.ToV8());
  } else {
    // No need to create native event if we do not need to send reply.
    event = CreateEvent(isolate);
  }

  Dictionary dict(isolate, event);
  dict.Set("sender", sender);
  // Should always set frameId even when callback is null.
  if (frame)
    dict.Set("frameId", frame->GetRoutingID());
  return event;
}