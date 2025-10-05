# Multithreaded-Web-Crawler

A fast, multi-threaded web crawler in C++ that extracts links from a given domain, supports depth and page limits, and saves results in JSON format.

This crawler uses **libcurl** for HTTP requests and **Gumbo parser** for HTML parsing. It is designed for quick and domain-specific web crawling.

---

## Features

- Multi-threaded crawling
- Domain-limited link extraction
- Depth-limited crawling
- Page limit control
- Results saved to `results.json`
- Lightweight and configurable

---

## Requirements

- C++17 compatible compiler
- [libcurl](https://curl.se/libcurl/)
- [Gumbo parser](https://github.com/google/gumbo-parser)
- CMake >= 3.16

---

## Build Instructions

```bash
git clone https://github.com/yourusername/web-crawler.git
cd web-crawler
mkdir build && cd build
cmake ..
make
./web_crawler
