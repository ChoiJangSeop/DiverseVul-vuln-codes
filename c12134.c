void WebContents::Message(bool internal,
                          const std::string& channel,
                          blink::CloneableMessage arguments,
                          content::RenderFrameHost* render_frame_host) {
  TRACE_EVENT1("electron", "WebContents::Message", "channel", channel);
  // webContents.emit('-ipc-message', new Event(), internal, channel,
  // arguments);
  EmitWithSender("-ipc-message", render_frame_host,
                 electron::mojom::ElectronBrowser::InvokeCallback(), internal,
                 channel, std::move(arguments));
}