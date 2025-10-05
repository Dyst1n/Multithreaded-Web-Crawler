#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <unordered_set>
#include <vector>
#include <atomic>
#include <fstream>
#include <chrono>
#include <regex>
#include <algorithm>

#include <curl/curl.h>
#include <gumbo.h>

struct CrawlTask {
    std::string url;
    int depth;
};


class SafeQueue {
public:
    void push(const CrawlTask& t) {
        std::lock_guard<std::mutex> lk(m_);
        q_.push(t);
        cv_.notify_one();
    }
    bool pop(CrawlTask& out) {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [&] { return !q_.empty() || finished_; });
        if (q_.empty()) return false;
        out = q_.front();
        q_.pop();
        return true;
    }
    void notify_finish() {
        std::lock_guard<std::mutex> lk(m_);
        finished_ = true;
        cv_.notify_all();
    }
    size_t size() {
        std::lock_guard<std::mutex> lk(m_);
        return q_.size();
    }
private:
    std::queue<CrawlTask> q_;
    std::mutex m_;
    std::condition_variable cv_;
    bool finished_ = false;
};

std::mutex cout_m;
void log_safe(const std::string& msg) {
    std::lock_guard<std::mutex> lk(cout_m);
    std::cout << msg << std::endl;
}


size_t write_cb(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


void search_links(GumboNode* node, std::vector<std::string>& links) {
    if (node->type != GUMBO_NODE_ELEMENT) return;
    if (node->v.element.tag == GUMBO_TAG_A) {
        GumboAttribute* href = gumbo_get_attribute(&node->v.element.attributes, "href");
        if (href) links.push_back(href->value);
    }
    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; i++)
        search_links((GumboNode*)children->data[i], links);
}

std::vector<std::string> extract_links(const std::string& html) {
    std::vector<std::string> links;
    GumboOutput* output = gumbo_parse(html.c_str());
    search_links(output->root, links);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return links;
}


std::string get_domain(const std::string& url) {
    std::regex re(R"(https?://([^/]+))", std::regex::icase);
    std::smatch m;
    if (std::regex_search(url, m, re)) return m[1];
    return "";
}
bool same_domain(const std::string& base, const std::string& other) {
    return get_domain(base) == get_domain(other);
}

int main() {
    std::string start_url;
    int threads = 4, max_depth = 2, max_pages = 50;

    std::cout << "=== Web Crawler ===\n\n=== Dyst1n (10/06/2025) === \n\nStart URL: ";
    std::getline(std::cin, start_url);
    std::cout << "Threads: "; std::cin >> threads;
    std::cout << "Max Depth: "; std::cin >> max_depth;
    std::cout << "Max Pages: "; std::cin >> max_pages;
    std::cout << "---------------------------------\n";

    curl_global_init(CURL_GLOBAL_DEFAULT);

    SafeQueue queue;
    std::unordered_set<std::string> visited;
    std::mutex visited_m;
    std::atomic<int> pages_crawled{ 0 };
    std::string domain = get_domain(start_url);

    queue.push({ start_url,0 });
    visited.insert(start_url);

    auto worker = [&]() {
        CURL* curl = curl_easy_init();
        if (!curl) return;
        CrawlTask task;
        while (queue.pop(task)) {
            if (max_pages > 0 && pages_crawled >= max_pages) break;

            long code = 0;
            log_safe("[fetch] " + task.url);
            std::string body;
            curl_easy_setopt(curl, CURLOPT_URL, task.url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Crawler/1.0");
            CURLcode res = curl_easy_perform(curl);
            if (res == CURLE_OK) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

            if (body.empty()) { log_safe("[err] " + task.url + " (" + std::to_string(code) + ")"); continue; }

            log_safe("[ok]  " + task.url + " (" + std::to_string(code) + ")");
            pages_crawled++;

            if (task.depth < max_depth) {
                auto links = extract_links(body);
                int added = 0;
                for (auto& l : links) {
                    if (l.rfind("http", 0) != 0) continue;
                    if (!same_domain(start_url, l)) continue;
                   
                    std::string norm = l;
                    std::transform(norm.begin(), norm.end(), norm.begin(), ::tolower);

                    std::lock_guard<std::mutex> lk(visited_m);
                    if (visited.insert(norm).second) {
                        queue.push({ l,task.depth + 1 });
                        added++;
                    }
                }
                log_safe("[links] " + std::to_string(added) + " new links");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        curl_easy_cleanup(curl);
        };

    std::vector<std::thread> workers;
    for (int i = 0; i < threads; i++) workers.emplace_back(worker);
    for (auto& t : workers) t.join();

    queue.notify_finish();
    curl_global_cleanup();

    
    std::ofstream f("results.json");
    f << "{\n  \"start\": \"" << start_url << "\",\n  \"count\": " << pages_crawled.load() << ",\n  \"urls\": [\n";
    bool first = true;
    for (auto& u : visited) {
        if (!first) f << ",\n"; f << "    \"" << u << "\""; first = false;
    }
    f << "\n  ]\n}\n";
    f.close();

    log_safe("\n[done] Crawling complete. " + std::to_string(pages_crawled.load()) + " pages saved to results.json");
    return 0;
}
