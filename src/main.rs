mod rest;

struct Configuration {
    port: u16,
    storage: String
}

#[tokio::main]
async fn main() {
    let config = Configuration::new();
    let bytes = bytes::Bytes::from("012345678");
    let hash = identity::sha256_digest_of_bytes(&bytes);

    println!("hash = {}", hash);
    rest::api::start(([0, 0, 0, 0], config.port), &config.storage).await;
}

impl Configuration {
    fn new() -> Configuration {
        let matches = clap::App::new("libernet")
                        .version("0.0.1")
                        .author("Marc Page <MarcAllenPage@gmail.com>")
                        .about("Wiki for the world")
                        .arg(clap::Arg::with_name("port")
                            .short("p")
                            .long("port")
                            .value_name("PORT")
                            .help("The port to listen on")
                        )
                        .arg(clap::Arg::with_name("storage")
                            .short("s")
                            .long("storage")
                            .value_name("STORAGE_PATH")
                            .help("The path to store objects")
                        ).get_matches();

        let port :u16 = matches.value_of("port").unwrap_or("8000").parse::<u16>().unwrap_or(8000);
        let storage = matches.value_of("storage").unwrap_or("/tmp").to_string();

        Configuration{port, storage}
    }
}

pub mod identity {

    fn digest_to_string(digest: ring::digest::Digest) -> String {
        let hash_strings = digest.as_ref().iter().map(|b| format!("{:02x?}", b));

        hash_strings.collect::<Vec<String>>().join("")
    }

    pub fn sha256_digest_of_bytes(contents: &bytes::Bytes) -> String {
        let mut context = ring::digest::Context::new(&ring::digest::SHA256);
        context.update(contents.as_ref());
        digest_to_string(context.finish())
    }

    pub fn read_sha256_digest<R: std::io::Read>(mut reader: R) -> Result<String, std::io::Error> {
        let mut context = ring::digest::Context::new(&ring::digest::SHA256);
        let mut buffer = [0; 1024];

        loop {
            let count = reader.read(&mut buffer)?;
            if count == 0 {
                break;
            }
            context.update(&buffer[..count]);
        }

        Ok(digest_to_string(context.finish()))
    }

}

