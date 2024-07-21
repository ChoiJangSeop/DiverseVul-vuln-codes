void WebContents::Invoke(
    bool internal,
    const std::string& channel,
    blink::CloneableMessage arguments,
    electron::mojom::ElectronBrowser::InvokeCallback callback,
    content::RenderFrameHost* render_frame_host) {
  TRACE_EVENT1("electron", "WebContents::Invoke", "channel", channel);
  // webContents.emit('-ipc-invoke', new Event(), internal, channel, arguments);
  EmitWithSender("-ipc-invoke", render_frame_host, std::move(callback),
                 internal, channel, std::move(arguments));
}