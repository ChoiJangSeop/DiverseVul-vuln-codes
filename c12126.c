void WebContents::MessageHost(const std::string& channel,
                              blink::CloneableMessage arguments,
                              content::RenderFrameHost* render_frame_host) {
  TRACE_EVENT1("electron", "WebContents::MessageHost", "channel", channel);
  // webContents.emit('ipc-message-host', new Event(), channel, args);
  EmitWithSender("ipc-message-host", render_frame_host,
                 electron::mojom::ElectronBrowser::InvokeCallback(), channel,
                 std::move(arguments));
}