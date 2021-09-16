pub mod api {
    use warp::Filter;

    fn is_local_link(addr: std::net::IpAddr) -> bool {
        let mut is_local = false;

        for iface in pnet::datalink::interfaces() {
            for address in iface.ips {
                if address.ip() == addr {
                    is_local = true
                }
            }
        }
        is_local
    }

    fn get_content(storage_path: &String, identifier: &String) -> String {
        let content_path = std::path::Path::new(storage_path)
            .join(identifier.chars().into_iter().take(2).collect::<String>())
            .join(identifier);
        format!("Path = {}", content_path.to_str().unwrap_or("???"))
    }

    pub async fn start(addr: impl Into<std::net::SocketAddr>, storage_path: &String) {
        println!("Path = {}", get_content(storage_path, &"hello".to_string()));

        // Private GET /api/<action>
        let api = warp::get()
            .and(warp::addr::remote())
            .and(warp::path::end()).map(|addr: Option<std::net::SocketAddr>| {
        if !is_local_link(addr.unwrap().ip()) {
            warp::http::Response::builder()
                .header("Content-Type", "text/html")
                .status(401)
                .body(format!("<html><body><b>Index App</b><br/><span style='color:red'>Not allowed</span><br/>From: {:?}</body></html>",addr))
        } else {
            warp::http::Response::builder()
                .header("Content-Type", "text/html")
                .status(401)
                .body(format!("<html><body><b>Index App</b><br/>From: {:?}</body></html>",addr))
        }

        // Private GET /api/<action>
        }).or(warp::get()
            .and(warp::addr::remote())
            .and(warp::path("api"))
            .and(warp::path::param())
            .and(warp::path::end())
            .map(|addr: Option<std::net::SocketAddr>, action: String| {
        if !is_local_link(addr.unwrap().ip()) {
            warp::http::Response::builder()
                .header("Content-Type", "text/html")
                .status(401)
                .body(format!("<html><body><b>Index App</b><br/><span style='color:red'>Not allowed</span><br/>From: {:?}</body></html>",addr))
        } else {
            warp::http::Response::builder()
                .header("Content-Type", "text/html")
                .body(format!("<html><body><b>Action: {}</b> @ {:?}</body></html>", action, addr))
        }

        // Private GET /app
        })).or(warp::get()
            .and(warp::addr::remote())
            .and(warp::path("app"))
            .and(warp::path::end())
            .map(|addr: Option<std::net::SocketAddr>| {
        if !is_local_link(addr.unwrap().ip()) {
            warp::http::Response::builder()
                .header("Content-Type", "text/html")
                .body(format!("<html><body><b>app Configure</b> ({:?})<span style='color:red'>Not allowed</span></body></html>", addr))
        } else {
            warp::http::Response::builder()
                .header("Content-Type", "text/html")
                .body(format!("<html><body><b>app Configure</b> ({:?})</body></html>", addr))
        }

        // Public GET /server
        })).or(warp::get()
            .and(warp::path("server"))
            .and(warp::path::end())
            .map(|| {
        warp::http::Response::builder()
            .header("Content-Type", "text/html")
            .body(format!("<html><body><b>server info</b></body></html>"))

        // Public PUT /server
        })).or(warp::put()
            .and(warp::body::content_length_limit(1024 * 1024))
            .and(warp::filters::body::bytes())
            .and(warp::path("server"))
            .and(warp::path::end())
            .map(|full_body: bytes::Bytes| {
        println!("bytes = {:?}", full_body);
        warp::http::Response::builder()
            .header("Content-Type", "text/html")
            .body(format!("<html><body><b>server info</b></body></html>"))

        // Public GET /data/requests
        })).or(warp::get()
            .and(warp::path("data"))
            .and(warp::path("requests"))
            .and(warp::path::end())
            .map(|| {
        warp::http::Response::builder()
            .header("Content-Type", "text/html")
            .body(format!("<html><body><b>data requests</b></body></html>"))

        // Public PUT /data/requests
        })).or(warp::put()
            .and(warp::body::content_length_limit(1024 * 1024))
            .and(warp::filters::body::bytes())
            .and(warp::path("data"))
            .and(warp::path("requests"))
            .and(warp::path::end())
            .map(|full_body: bytes::Bytes| {
        println!("bytes = {:?}", full_body);
        warp::http::Response::builder()
            .header("Content-Type", "text/html")
            .body(format!("<html><body><b>data requests</b></body></html>"))

        // Public GET /data/sha256/<hash>
        })).or(warp::get()
            .and(warp::path("data"))
            .and(warp::path("sha256"))
            .and(warp::path::param())
            .and(warp::path::end())
            .map(|hash: String| {
        warp::http::Response::builder()
            .header("Content-Type", "text/html")
            .body(format!("<html><body><b>data: hash={}</b></body></html>", hash))

        // Public PUT /data/sha256/<hash>
        })).or(warp::put()
            .and(warp::body::content_length_limit(1024 * 1024))
            .and(warp::filters::body::bytes())
            .and(warp::path("data"))
            .and(warp::path("sha256"))
            .and(warp::path::param())
            .and(warp::path::end())
            .map(|full_body: bytes::Bytes, hash: String| {
        println!("bytes = {:?}", full_body);
        warp::http::Response::builder()
            .header("Content-Type", "text/html")
            .body(format!("<html><body><b>data: hash={}</b></body></html>", hash))

        // Public GET /data/like/sha256/<hash>
        })).or(warp::get()
            .and(warp::path("data"))
            .and(warp::path("like"))
            .and(warp::path("sha256"))
            .and(warp::path::param())
            .and(warp::path::end())
            .map(|hash: String| {
        warp::http::Response::builder()
            .header("Content-Type", "text/html")
            .body(format!("<html><body><b>data like: hash={}</b></body></html>", hash))

        // Public GET /data/sha256/<hash>/aes256/<key>
        })).or(warp::get()
            .and(warp::path("data"))
            .and(warp::path("sha256"))
            .and(warp::path::param())
            .and(warp::path("aes256"))
            .and(warp::path::param())
            .and(warp::path::end())
            .map(|hash: String, key: String| {
        warp::http::Response::builder()
            .header("Content-Type", "text/html")
            .body(format!("<html><body><b>data: hash={} key={}</b></body></html>", hash, key))

        // Private GET /web/<path>
        })).or(warp::get()
            .and(warp::addr::remote())
            .and(warp::path("web"))
            .and(warp::path::tail())
            .map(|addr: Option<std::net::SocketAddr>, path: warp::path::Tail| {
        if !is_local_link(addr.unwrap().ip()) {
            warp::http::Response::builder()
                .header("Content-Type", "text/html")
                .body(format!("<html><body><b>web</b>: {}<br/><span style='color:red'>Not allowed</span></body></html>", path.as_str()))
        } else {
            warp::http::Response::builder()
                .header("Content-Type", "text/html")
                .body(format!("<html><body><b>web</b>: {}</body></html>", path.as_str()))
        }

        // Private GET /<app>/<app path>
        })).or(warp::get()
            .and(warp::addr::remote())
            .and(warp::path::param())
            .and(warp::path::tail())
            .map(|addr: Option<std::net::SocketAddr>, app_name: String, path: warp::path::Tail| {
        if !is_local_link(addr.unwrap().ip()) {
            warp::http::Response::builder()
                .header("Content-Type", "text/html")
                .body(format!("<html><body><b>{}</b>: {}<br/><span style='color:red'>Not allowed</span></body></html>", app_name, path.as_str()))
        } else {
            warp::http::Response::builder()
                .header("Content-Type", "text/html")
                .body(format!("<html><body><b>{}</b>: {}</body></html>", app_name, path.as_str()))
        }

        }));

        warp::serve(api).run(addr).await;
    }
}
