#ifndef __MimeTypes_h__
#define __MimeTypes_h__

namespace mime {

struct ExtensionMapping {
  const char *const extension;
  const char *const type;
};

inline std::string &fromExtension(const std::string &extension,
                                  const std::string &mimeType) {
  static const ExtensionMapping extensions[] = {
      {"3g2", "video/3gpp2"},
      {"3gp", "video/3gpp"},
      {"7z", "application/x-7z-compressed"},
      {"aac", "audio/aac"},
      {"abw", "application/x-abiword"},
      {"arc", "application/octet-stream"},
      {"avi", "video/x-msvideo"},
      {"avi", "video/x-msvideo"},
      {"azw", "application/vnd.amazon.ebook"},
      {"bin", "application/octet-stream"},
      {"bmp", "image/bmp"},
      {"bz", "application/x-bzip"},
      {"bz2", "application/x-bzip2"},
      {"csh", "application/x-csh"},
      {"css", "text/css"},
      {"csv", "text/csv"},
      {"doc", "application/msword"},
      {"docx", "application/"
               "vnd.openxmlformats-officedocument.wordprocessingml.document"},
      {"eot", "application/vnd.ms-fontobject"},
      {"epub", "application/epub+zip"},
      {"es", "application/ecmascript"},
      {"flv", "video/x-flv"},
      {"gif", "image/gif"},
      {"htm", "text/html"},
      {"html", "text/html"},
      {"ico", "image/x-icon"},
      {"ics", "text/calendar"},
      {"jar", "application/java-archive"},
      {"jpeg", "image/jpeg"},
      {"jpg", "image/jpeg"},
      {"js", "application/javascript"},
      {"json", "application/json"},
      {"m3u8", "application/x-mpegURL"},
      {"mid", "audio/midi audio/x-midi"},
      {"midi", "audio/midi audio/x-midi"},
      {"mov", "video/quicktime"},
      {"mp4", "video/mp4"},
      {"mpeg", "video/mpeg"},
      {"mpkg", "application/vnd.apple.installer+xml"},
      {"odp", "application/vnd.oasis.opendocument.presentation"},
      {"ods", "application/vnd.oasis.opendocument.spreadsheet"},
      {"odt", "application/vnd.oasis.opendocument.text"},
      {"oga", "audio/ogg"},
      {"ogv", "video/ogg"},
      {"ogx", "application/ogg"},
      {"otf", "font/otf"},
      {"pdf", "application/pdf"},
      {"png", "image/png"},
      {"ppt", "application/vnd.ms-powerpoint"},
      {"pptx", "application/"
               "vnd.openxmlformats-officedocument.presentationml.presentation"},
      {"rar", "application/x-rar-compressed"},
      {"rtf", "application/rtf"},
      {"sh", "application/x-sh"},
      {"svg", "image/svg+xml"},
      {"swf", "application/x-shockwave-flash"},
      {"tar", "application/x-tar"},
      {"tif", "image/tiff"},
      {"tiff", "image/tiff"},
      {"ts", "application/typescript"},
      {"ts", "video/MP2T"},
      {"ttf", "font/ttf"},
      {"txt", "text/plain"},
      {"vsd", "application/vnd.visio"},
      {"wav", "audio/wav"},
      {"weba", "audio/webm"},
      {"webm", "video/webm"},
      {"webp", "image/webp"},
      {"wmv", "video/x-ms-wmv"},
      {"woff", "font/woff"},
      {"woff2", "font/woff2"},
      {"xhtml", "application/xhtml+xml"},
      {"xls", "application/vnd.ms-excel"},
      {"xlsx",
       "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
      {"xml", "application/xml"},
      {"xul", "application/vnd.mozilla.xul+xml"},
      {"zip", "application/zip"},
  };
  static const int count =
      sizeof(mime::extensions) / sizeof(mime::extensions[0]);

  for (int i = 0; i < count; ++i) {
    if (extension == extensions[i].extension) {
      return mimeType = extensions[i].type;
    }
  }
  return mimeType = "";
}

inline std::string fromExtension(const std::string &extension) {
  std::string buffer;

  return fromExtension(extension, buffer);
}

} // namespace mime

#endif // __MimeTypes_h__
