use clap::{Arg, App};
use warp::Filter;

#[tokio::main]
async fn main() {
    // GET /hello/warp => 200 OK with body "Hello, warp!"
    let matches = App::new("libernet")
                    .version("0.0.1")
                    .author("Marc Page <MarcAllenPage@gmail.com")
                    .about("Wiki for the world")
                    .arg(Arg::with_name("port")
                        .short("p")
                        .long("port")
                        .value_name("PORT")
                        .help("The port to listen on")).get_matches();
    // implement /web, /{app}, and PUT for /server, /data/sha256/{hash}
    let rest = warp::path::end().map(|| {
        warp::http::Response::builder()
            .header("Content-Type", "text/html")
            .body("<html><body><b>Index App</b></body></html>")
        }).or(warp::path("api")
            .and(warp::path::param())
            .and(warp::path::end())
            .map(|action: String| {
        warp::http::Response::builder()
            .header("Content-Type", "text/html")
            .body(format!("<html><body><b>Action: {}</b></body></html>", action))
        })).or(warp::path("app")
            .and(warp::path::end())
            .map(|| {
        warp::http::Response::builder()
            .header("Content-Type", "text/html")
            .body(format!("<html><body><b>app Configure</b></body></html>"))
        })).or(warp::path("server")
            .and(warp::path::end())
            .map(|| {
        warp::http::Response::builder()
            .header("Content-Type", "text/html")
            .body(format!("<html><body><b>server info</b></body></html>"))
        })).or(warp::path("data")
            .and(warp::path("requests"))
            .and(warp::path::end())
            .map(|| {
        warp::http::Response::builder()
            .header("Content-Type", "text/html")
            .body(format!("<html><body><b>data requests</b></body></html>"))
        })).or(warp::path("data")
            .and(warp::path("sha256"))
            .and(warp::path::param())
            .and(warp::path::end())
            .map(|hash: String| {
        warp::http::Response::builder()
            .header("Content-Type", "text/html")
            .body(format!("<html><body><b>data: hash={}</b></body></html>", hash))
        })).or(warp::path("data")
            .and(warp::path("like"))
            .and(warp::path("sha256"))
            .and(warp::path::param())
            .and(warp::path::end())
            .map(|hash: String| {
        warp::http::Response::builder()
            .header("Content-Type", "text/html")
            .body(format!("<html><body><b>data like: hash={}</b></body></html>", hash))
        })).or(warp::path("data")
            .and(warp::path("sha256"))
            .and(warp::path::param())
            .and(warp::path("aes256"))
            .and(warp::path::param())
            .and(warp::path::end())
            .map(|hash: String, key: String| {
        warp::http::Response::builder()
            .header("Content-Type", "text/html")
            .body(format!("<html><body><b>data: hash={} key={}</b></body></html>", hash, key))
        }));

    let port:u16 = matches.value_of("port").unwrap_or("8000").parse::<u16>().unwrap_or(80);
    warp::serve(rest)
        .run(([127, 0, 0, 1], port))
        .await;
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
    let matches = App::new("libernet")
                    .version("0.0.1")
                    .author("Marc Page <MarcAllenPage@gmail.com")
                    .about("Wiki for the world")
                    .arg(Arg::with_name("port")
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
