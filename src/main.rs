mod rest;

pub use crate::rest::api;

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

    let port:u16 = matches.value_of("port").unwrap_or("8000").parse::<u16>().unwrap_or(8000);

    rest::api::start(port).await;
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
