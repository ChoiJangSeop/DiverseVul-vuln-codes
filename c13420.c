bool WebContents::SendIPCMessageToFrame(bool internal,
                                        int32_t frame_id,
                                        const std::string& channel,
                                        v8::Local<v8::Value> args) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  blink::CloneableMessage message;
  if (!gin::ConvertFromV8(isolate, args, &message)) {
    isolate->ThrowException(v8::Exception::Error(
        gin::StringToV8(isolate, "Failed to serialize arguments")));
    return false;
  }
  auto frames = web_contents()->GetAllFrames();
  auto iter = std::find_if(frames.begin(), frames.end(), [frame_id](auto* f) {
    return f->GetRoutingID() == frame_id;
  });
  if (iter == frames.end())
    return false;
  if (!(*iter)->IsRenderFrameLive())
    return false;

  mojo::AssociatedRemote<mojom::ElectronRenderer> electron_renderer;
  (*iter)->GetRemoteAssociatedInterfaces()->GetInterface(&electron_renderer);
  electron_renderer->Message(internal, channel, std::move(message),
                             0 /* sender_id */);
  return true;
}