v8::Local<v8::Object> CreateNativeEvent(
    v8::Isolate* isolate,
    v8::Local<v8::Object> sender,
    content::RenderFrameHost* frame,
    electron::mojom::ElectronBrowser::MessageSyncCallback callback) {
  v8::Local<v8::Object> event;
  if (frame && callback) {
    gin::Handle<Event> native_event = Event::Create(isolate);
    native_event->SetCallback(std::move(callback));
    event = native_event.ToV8().As<v8::Object>();
  } else {
    // No need to create native event if we do not need to send reply.
    event = CreateEvent(isolate);
  }

  Dictionary dict(isolate, event);
  dict.Set("sender", sender);
  // Should always set frameId even when callback is null.
  if (frame) {
    dict.Set("frameId", frame->GetRoutingID());
    dict.Set("processId", frame->GetProcess()->GetID());
  }
  return event;
}