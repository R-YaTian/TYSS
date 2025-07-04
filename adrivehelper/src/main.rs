use wry::WebViewBuilder;
use winit::{
    application::ApplicationHandler,
    event::WindowEvent,
    event_loop::{ActiveEventLoop, EventLoop},
    window::{Window, WindowId},
};
use serde_json::{json, Value};
use std::fs;
use url::Url;

// For Windows MessageBox
use std::ffi::OsStr;
use std::iter::once;
use std::os::windows::ffi::OsStrExt;
use winapi::um::winuser::{MessageBoxW, MB_OK};

fn message_box(title: &str, message: &str) {
    let to_wstring = |s: &str| {
        OsStr::new(s).encode_wide().chain(once(0)).collect::<Vec<u16>>()
    };

    let title_w = to_wstring(title);
    let message_w = to_wstring(message);

    unsafe {
        MessageBoxW(
            std::ptr::null_mut(),
            message_w.as_ptr(),
            title_w.as_ptr(),
            MB_OK,
        );
    }
}

#[derive(Default)]
struct App {
    window: Option<Window>,
    webview: Option<wry::WebView>,
}

impl ApplicationHandler for App {
    fn resumed(&mut self, event_loop: &ActiveEventLoop) {
        let window = event_loop
            .create_window(
                Window::default_attributes()
                    .with_title("ADrive OAuth Page")
            )
            .unwrap();

        let oauth_url = "https://openapi.alipan.com/oauth/authorize?client_id=0f2cda4bb8de4f669ef4d3d763e88738&redirect_uri=oob&scope=user:base,file:all:write,file:all:read&style=all";

        let webview = WebViewBuilder::new()
            .with_url(oauth_url)
            .with_ipc_handler(|request| {
                // IPC Message From JS
                println!("Received IPC Message: {}", request.body());
                // Parse JSON Message
                if let Ok(message) = serde_json::from_str::<Value>(request.body()) {
                    if let Some(msg_type) = message["type"].as_str() {
                        if msg_type == "oauth_callback" {
                            if let Some(callback_url) = message["url"].as_str() {
                                // Parse URL then get auth code
                                if let Ok(parsed_url) = Url::parse(callback_url) {
                                    if let Some(query_pairs) = parsed_url.query() {
                                        for (key, value) in url::form_urlencoded::parse(query_pairs.as_bytes()) {
                                            if key == "code" {
                                                // Create drive.json file
                                                let drive_config = json!({
                                                    "driveAuthCode": value.to_string()
                                                });
                                                // Write drive.json file
                                                if let Err(e) = fs::write("drive.json", drive_config.to_string()) {
                                                    eprintln!("Failed to write drive.json: {}", e);
                                                } else {
                                                    message_box("授权成功！", "授权码已保存到 drive.json 文件\n请将其放到SD卡根目录的TYSS文件夹中,并在10分钟之内启动TYSS完成客户端授权!");
                                                }
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            })
            .with_initialization_script(
                r#"
                let originalPushState = history.pushState;
                let originalReplaceState = history.replaceState;

                function checkUrl() {
                    const currentUrl = window.location.href;
                    if (currentUrl.startsWith('https://openapi.alipan.com/oauth/authorize/callback?')) {
                        // Send Message to Rust
                        window.ipc.postMessage(JSON.stringify({
                            type: 'oauth_callback',
                            url: currentUrl
                        }));
                    }
                }

                // Overwrite history API
                history.pushState = function() {
                    originalPushState.apply(history, arguments);
                    setTimeout(checkUrl, 0);
                };

                history.replaceState = function() {
                    originalReplaceState.apply(history, arguments);
                    setTimeout(checkUrl, 0);
                };

                // Listen popstate Event
                window.addEventListener('popstate', checkUrl);

                // Listen load Event
                window.addEventListener('load', checkUrl);
                "#
            )
            .build(&window)
            .unwrap();

        self.window = Some(window);
        self.webview = Some(webview);
    }

    fn window_event(
        &mut self,
        event_loop: &ActiveEventLoop,
        _window_id: WindowId,
        event: WindowEvent,
    ) {
        match event {
            WindowEvent::CloseRequested => {
                event_loop.exit();
            }
            _ => {}
        }
    }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let event_loop = EventLoop::new()?;
    let mut app = App::default();
    event_loop.run_app(&mut app)?;
    Ok(())
}
