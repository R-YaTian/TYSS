use tiny_http::{Server, Response};
use webbrowser;
use std::fs::File;
use std::io::Write;
use std::env;

fn main() {
    let port = 10304;
    let redirect_uri = format!("http://127.0.0.1:{}/callback", port);

    let client_id = "0f2cda4bb8de4f669ef4d3d763e88738";
    let scope = "user:base,file:all:write,file:all:read";
    let style = "all";

    let auth_url = format!(
        "https://openapi.alipan.com/oauth/authorize?client_id={}&redirect_uri={}&scope={}&style={}&response_type=code",
        client_id, redirect_uri, scope, style
    );

    println!("Opening browser for AliDrive authorization...");
    if webbrowser::open(&auth_url).is_err() {
        println!("Failed to open browser!\nPlease manually visit this URL:\n{}", auth_url);
    }

    println!("Listening for callback at: {}", redirect_uri);
    let server = Server::http(format!("0.0.0.0:{}", port)).unwrap();

    for request in server.incoming_requests() {
        let url = request.url().to_string();

        if url.starts_with("/callback") {
            // Extract code from URL
            if let Some(code_part) = url.split("code=").nth(1) {
                let code = code_part.split('&').next().unwrap_or("").to_string();

                println!("Received auth code: {}", code);

                // Write to drive.json
                if let Err(e) = save_auth_code(&code) {
                    eprintln!("Failed to write drive.json: {}", e);
                } else {
                    println!("Saved auth code to drive.json");
                }

                // Respond to browser
                let response = Response::from_string("授权成功!\n授权码已保存到 drive.json 文件\n请将其放到SD卡根目录的TYSS文件夹中,并在10分钟之内启动TYSS完成客户端授权!\n此网页可安全关闭。");
                let _ = request.respond(response);

                println!("操作已完成, 请按下回车退出程序...");
                let _ = std::io::stdin().read_line(&mut String::new());
                break; // Stop server after getting code
            } else {
                let response = Response::from_string("Missing code in callback URL.");
                let _ = request.respond(response);
            }
        } else {
            let response = Response::from_string("Not found.");
            let _ = request.respond(response);
        }
    }
}

fn save_auth_code(code: &str) -> std::io::Result<()> {
    let exe_path = env::current_exe()?;
    let exe_dir = exe_path.parent().unwrap();

    let file_path = exe_dir.join("drive.json");
    let content = format!("{{ \"driveAuthCode\": \"{}\" }}", code);

    let mut file = File::create(file_path)?;
    file.write_all(content.as_bytes())?;
    Ok(())
}
