#pragma once
#include <cctype>
#include <string>

struct MimePair {
	const char *ext;
	const char *mime;
};

std::string mime_from_path(const std::string &p) {
	static const MimePair table[] = {// --- Text ---
					 {"txt", "text/plain"},
					 {"html", "text/html"},
					 {"htm", "text/html"},
					 {"css", "text/css"},
					 {"csv", "text/csv"},
					 {"js", "application/javascript"},
					 {"json", "application/json"},
					 {"xml", "application/xml"},

					 // --- Images ---
					 {"png", "image/png"},
					 {"jpg", "image/jpeg"},
					 {"jpeg", "image/jpeg"},
					 {"gif", "image/gif"},
					 {"bmp", "image/bmp"},
					 {"ico", "image/x-icon"},
					 {"svg", "image/svg+xml"},
					 {"webp", "image/webp"},
					 {"tif", "image/tiff"},
					 {"tiff", "image/tiff"},

					 // --- Audio ---
					 {"mp3", "audio/mpeg"},
					 {"wav", "audio/wav"},
					 {"ogg", "audio/ogg"},
					 {"flac", "audio/flac"},
					 {"m4a", "audio/mp4"},

					 // --- Video ---
					 {"mp4", "video/mp4"},
					 {"webm", "video/webm"},
					 {"mov", "video/quicktime"},
					 {"avi", "video/x-msvideo"},
					 {"mkv", "video/x-matroska"},

					 // --- Fonts ---
					 {"woff", "font/woff"},
					 {"woff2", "font/woff2"},
					 {"ttf", "font/ttf"},
					 {"otf", "font/otf"},

					 // --- Binary ---
					 {"pdf", "application/pdf"},
					 {"wasm", "application/wasm"},
					 {"zip", "application/zip"},
					 {"gz", "application/gzip"},
					 {"tar", "application/x-tar"},
					 {"rar", "application/vnd.rar"}};

	static const size_t tableSize = sizeof(table) / sizeof(table[0]);

	size_t dot = p.rfind('.');
	if (dot == std::string::npos)
		return "application/octet-stream";

	std::string ext = p.substr(dot + 1);

	for (size_t i = 0; i < ext.size(); ++i)
		ext[i] = static_cast<char>(std::tolower(ext[i]));

	for (size_t i = 0; i < tableSize; ++i) {
		if (ext == table[i].ext)
			return table[i].mime;
	}
	return "application/octet-stream";
}
