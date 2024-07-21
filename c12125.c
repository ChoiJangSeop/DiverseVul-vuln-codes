void WebContents::ReceivePostMessage(
    const std::string& channel,
    blink::TransferableMessage message,
    content::RenderFrameHost* render_frame_host) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  auto wrapped_ports =
      MessagePort::EntanglePorts(isolate, std::move(message.ports));
  v8::Local<v8::Value> message_value =
      electron::DeserializeV8Value(isolate, message);
  EmitWithSender("-ipc-ports", render_frame_host,
                 electron::mojom::ElectronBrowser::InvokeCallback(), false,
                 channel, message_value, std::move(wrapped_ports));
}