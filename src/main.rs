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

#[tokio::main]
async fn main() {
    // GET /hello/warp => 200 OK with body "Hello, warp!"
    let matches = clap::App::new("libernet")
                    .version("0.0.1")
                    .author("Marc Page <MarcAllenPage@gmail.com")
                    .about("Wiki for the world")
                    .arg(clap::Arg::with_name("port")
                        .short("p")
                        .long("port")
                        .value_name("PORT")
                        .help("The port to listen on")).get_matches();

    // Private GET root (/)
    let rest = warp::get()
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
            .and(warp::path("server"))
            .and(warp::path::end())
            .map(|| {
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
            .and(warp::path("data"))
            .and(warp::path("sha256"))
            .and(warp::path::param())
            .and(warp::path::end())
            .map(|hash: String| {
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

    let port:u16 = matches.value_of("port").unwrap_or("8000").parse::<u16>().unwrap_or(80);
    warp::serve(rest)
        .run(([0, 0, 0, 0], port))
        .await;
}

pub mod identity {

    pub fn sha256_digest<R: std::io::Read>(mut reader: R) -> Result<ring::digest::Digest, std::io::Error> {
        let mut context = ring::digest::Context::new(&ring::digest::SHA256);
        let mut buffer = [0; 1024];

        loop {
            let count = reader.read(&mut buffer)?;
            if count == 0 {
                break;
            }
            context.update(&buffer[..count]);
        }

        Ok(context.finish())
    }

}
/*
error_chain::error_chain! {
    foreign_links {
        Io(std::io::Error);
        Decode(data_encoding::DecodeError);
    }
}



    pub fn sha256_digest<R: std::io::Read>(mut reader: R) -> Result<ring::digest::Digest> {
        let mut context = ring::digest::Context::new(&ring::digest::SHA256);
        let mut buffer = [0; 1024];

        loop {
            let count = reader.read(&mut buffer)?;
            if count == 0 {
                break;
            }
            context.update(&buffer[..count]);
        }

        Ok(context.finish())
    }


use std::io::Write;

fn main() -> Result<()> {
    let matches = clap::App::new("libernet")
                    .version("0.0.1")
                    .author("Marc Page <MarcAllenPage@gmail.com")
                    .about("Wiki for the world")
                    .arg(clap::Arg::with_name("port")
                        .short("p")
                        .long("port")
                        .value_name("PORT")
                        .help("The port to listen on")).get_matches();

    let port = matches.value_of("port").unwrap_or("80");
    println!("Listening on port {}", port);

    let path = "/tmp/file.txt";

    let mut output = std::fs::File::create(path)?;
    write!(output, "We will generate a digest of this text")?;

    let input = std::fs::File::open(path)?;
    let reader = std::io::BufReader::new(input);
    let digest = sha256_digest(reader)?;

    println!("SHA-256 digest is {}", data_encoding::HEXLOWER.encode(digest.as_ref()));

    Ok(())
}

*/
