#include "dsa/HttpServer.hpp"
#include "dsa/Algorithms.hpp"
#include "dsa/CodeRegistry.hpp"
#include "dsa/Json.hpp"

#include <arpa/inet.h>
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

namespace dsa::server {

namespace {

using dsa::json::JsonValue;

struct JsonParser {
    explicit JsonParser(const std::string& src) : src_(src) {}
    JsonValue parse() const { return dsa::json::parse(src_); }
    const std::string& src_;
};

inline std::string jsonEscape(const std::string& s) { return dsa::json::escape(s); }
inline std::string jsonIntArray(const std::vector<int>& arr) { return dsa::json::intArray(arr); }

std::string serializeInfo(const AlgorithmInfo& info) {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":" << jsonEscape(info.id) << ","
        << "\"name\":" << jsonEscape(info.name) << ","
        << "\"category\":" << jsonEscape(info.category) << ","
        << "\"description\":" << jsonEscape(info.description) << ","
        << "\"bestTime\":" << jsonEscape(info.bestTime) << ","
        << "\"averageTime\":" << jsonEscape(info.averageTime) << ","
        << "\"worstTime\":" << jsonEscape(info.worstTime) << ","
        << "\"spaceComplexity\":" << jsonEscape(info.spaceComplexity) << ","
        << "\"notes\":" << jsonEscape(info.notes)
        << "}";
    return oss.str();
}

std::string jsonStringArray(const std::vector<std::string>& arr) {
    std::ostringstream oss;
    oss << "[";
    for (std::size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) oss << ",";
        oss << jsonEscape(arr[i]);
    }
    oss << "]";
    return oss.str();
}

std::string serializeBstResult(const BstOperationResult& res) {
    std::ostringstream oss;
    oss << "{"
        << "\"ok\":true,"
        << "\"success\":" << (res.success ? "true" : "false") << ","
        << "\"operation\":" << jsonEscape(res.operation) << ","
        << "\"message\":" << jsonEscape(res.message) << ","
        << "\"traversalOrder\":" << jsonIntArray(res.traversalOrder) << ","
        << "\"nodeCount\":" << res.nodeCount << ","
        << "\"height\":" << res.height << ","
        << "\"steps\":[";
    for (std::size_t i = 0; i < res.steps.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& st = res.steps[i];
        oss << "{"
            << "\"value\":" << st.value << ","
            << "\"action\":" << jsonEscape(st.action) << ","
            << "\"detail\":" << jsonEscape(st.detail)
            << "}";
    }
    oss << "],\"nodes\":[";
    for (std::size_t i = 0; i < res.nodes.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& n = res.nodes[i];
        oss << "{"
            << "\"value\":" << n.value << ","
            << "\"depth\":" << n.depth << ","
            << "\"orderIndex\":" << n.orderIndex << ","
            << "\"hasLeft\":" << (n.hasLeft ? "true" : "false") << ","
            << "\"leftValue\":" << n.leftValue << ","
            << "\"hasRight\":" << (n.hasRight ? "true" : "false") << ","
            << "\"rightValue\":" << n.rightValue
            << "}";
    }
    oss << "]}";
    return oss.str();
}

std::string serializeGraphSnapshot(const GraphSnapshot& snap) {
    std::ostringstream oss;
    oss << "{"
        << "\"directed\":" << (snap.directed ? "true" : "false") << ","
        << "\"vertices\":" << jsonStringArray(snap.vertices) << ","
        << "\"edges\":[";
    for (std::size_t i = 0; i < snap.edges.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{\"from\":" << jsonEscape(snap.edges[i].from)
            << ",\"to\":" << jsonEscape(snap.edges[i].to) << "}";
    }
    oss << "]}";
    return oss.str();
}

std::string serializeProgress(const ProgressSnapshot& snap) {
    std::ostringstream oss;
    oss << std::fixed;
    oss.precision(1);
    oss << "{"
        << "\"completedCount\":" << snap.completedCount << ","
        << "\"totalModules\":" << snap.totalModules << ","
        << "\"completionPercent\":" << snap.completionPercent << ","
        << "\"totalQuizzesAttempted\":" << snap.totalQuizzesAttempted << ","
        << "\"averageAccuracy\":" << snap.averageAccuracy << ","
        << "\"bestAccuracy\":" << snap.bestAccuracy << ","
        << "\"modules\":[";
    for (std::size_t i = 0; i < snap.modules.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{"
            << "\"id\":" << jsonEscape(snap.modules[i].id) << ","
            << "\"name\":" << jsonEscape(snap.modules[i].name) << ","
            << "\"completed\":" << (snap.modules[i].completed ? "true" : "false")
            << "}";
    }
    oss << "],\"quizHistory\":[";
    for (std::size_t i = 0; i < snap.quizHistory.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& q = snap.quizHistory[i];
        oss << "{"
            << "\"timestamp\":" << jsonEscape(q.timestamp) << ","
            << "\"category\":" << jsonEscape(q.category) << ","
            << "\"score\":" << q.score << ","
            << "\"total\":" << q.total << ","
            << "\"accuracy\":" << q.accuracy
            << "}";
    }
    oss << "]}";
    return oss.str();
}

std::string serializeDsState(const DataStructureState& st) {
    std::ostringstream oss;
    oss << "{"
        << "\"name\":" << jsonEscape(st.name) << ","
        << "\"elements\":" << jsonIntArray(st.elements) << ","
        << "\"size\":" << st.size << ","
        << "\"capacity\":" << st.capacity << ","
        << "\"isEmpty\":" << (st.isEmpty ? "true" : "false") << ","
        << "\"isFull\":" << (st.isFull ? "true" : "false") << ","
        << "\"hasTopOrFront\":" << (st.hasTopOrFront ? "true" : "false") << ","
        << "\"topOrFrontValue\":" << st.topOrFrontValue
        << "}";
    return oss.str();
}

std::string toLower(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string statusText(int code) {
    switch (code) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        default: return "OK";
    }
}

std::string mimeTypeForPath(const std::filesystem::path& p) {
    const std::string ext = toLower(p.extension().string());
    if (ext == ".html") return "text/html; charset=utf-8";
    if (ext == ".css") return "text/css; charset=utf-8";
    if (ext == ".js") return "application/javascript; charset=utf-8";
    if (ext == ".json") return "application/json; charset=utf-8";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".png") return "image/png";
    return "text/plain; charset=utf-8";
}

bool sendAll(int fd, const char* data, std::size_t length) {
    std::size_t sent = 0;
    while (sent < length) {
        ssize_t n = ::send(fd, data + sent, length - sent, 0);
        if (n <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

} // namespace

// ============================================================================
// HttpServer Lifecycle
// ============================================================================

HttpServer::HttpServer(std::uint16_t port, std::string webRoot, HistoryLogger& logger,
                       ProgressManager& progress, LearningHub& hub, IntelligenceHub& intel)
    : port_(port),
      webRoot_(std::move(webRoot)),
      logger_(logger),
      progress_(progress),
      hub_(hub),
      intel_(intel),
      stack_(8),
      queue_(8),
      linkedList_(10),
      graph_(false) {
    linkedList_.loadSample();
    bst_.loadSample();
    graph_.loadSample();
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    std::signal(SIGPIPE, SIG_IGN);

    serverFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ < 0) {
        throw std::runtime_error("Failed to create listening socket.");
    }

    int opt = 1;
    ::setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(port_);

    if (::bind(serverFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        int err = errno;
        ::close(serverFd_);
        serverFd_ = -1;
        throw std::runtime_error("Failed to bind to 127.0.0.1:" + std::to_string(port_) +
                                 " (errno=" + std::to_string(err) + ").");
    }

    if (::listen(serverFd_, 32) < 0) {
        ::close(serverFd_);
        serverFd_ = -1;
        throw std::runtime_error("Failed to listen on socket.");
    }

    running_.store(true);
    std::cout << "DSA Manager Lite running at http://127.0.0.1:" << port_ << "\n"
              << "Serving frontend from: " << webRoot_ << "\n"
              << "Logging operations to: " << logger_.filePath() << std::endl;

    while (running_.load()) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = ::accept(serverFd_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientFd < 0) {
            if (!running_.load()) break;
            if (errno == EINTR) continue;
            continue;
        }
        handleClient(clientFd);
        ::close(clientFd);
    }
}

void HttpServer::stop() {
    running_.store(false);
    if (serverFd_ >= 0) {
        ::shutdown(serverFd_, SHUT_RDWR);
        ::close(serverFd_);
        serverFd_ = -1;
    }
}

void HttpServer::handleClient(int clientFd) {
    timeval tv{};
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    ::setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    std::string raw;
    char buf[4096];
    std::size_t headerEnd = std::string::npos;

    while (raw.size() < 65536) {
        ssize_t n = ::recv(clientFd, buf, sizeof(buf), 0);
        if (n <= 0) break;
        raw.append(buf, static_cast<std::size_t>(n));
        headerEnd = raw.find("\r\n\r\n");
        if (headerEnd != std::string::npos) break;
    }

    if (headerEnd == std::string::npos) {
        return;
    }

    const std::string headerBlock = raw.substr(0, headerEnd);
    std::istringstream headerStream(headerBlock);
    std::string requestLine;
    std::getline(headerStream, requestLine);
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    std::istringstream rlStream(requestLine);
    HttpRequest req;
    std::string version;
    rlStream >> req.method >> req.path >> version;

    const std::size_t qPos = req.path.find('?');
    if (qPos != std::string::npos) {
        req.path = req.path.substr(0, qPos);
    }

    std::size_t contentLength = 0;
    std::string line;
    while (std::getline(headerStream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const std::size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = toLower(line.substr(0, colon));
        std::string val = line.substr(colon + 1);
        const std::size_t firstChar = val.find_first_not_of(" \t");
        if (firstChar != std::string::npos) {
            val = val.substr(firstChar);
        } else {
            val.clear();
        }
        if (key == "content-length") {
            try {
                contentLength = static_cast<std::size_t>(std::stoul(val));
            } catch (...) {
                contentLength = 0;
            }
        }
    }

    if (contentLength > 16777216) {
        HttpResponse errResp{400, "application/json; charset=utf-8",
                             "{\"ok\":false,\"code\":\"PAYLOAD_TOO_LARGE\",\"error\":\"Request body too large.\"}"};
        std::ostringstream out;
        out << "HTTP/1.1 400 Bad Request\r\nContent-Type: " << errResp.contentType
            << "\r\nContent-Length: " << errResp.body.size()
            << "\r\nAccess-Control-Allow-Origin: *"
            << "\r\nConnection: close\r\n\r\n"
            << errResp.body;
        const std::string wire = out.str();
        sendAll(clientFd, wire.data(), wire.size());
        return;
    }

    req.body = raw.substr(headerEnd + 4);
    while (req.body.size() < contentLength) {
        ssize_t n = ::recv(clientFd, buf, sizeof(buf), 0);
        if (n <= 0) break;
        req.body.append(buf, static_cast<std::size_t>(n));
    }
    if (req.body.size() > contentLength) {
        req.body.resize(contentLength);
    }

    HttpResponse resp = routeRequest(req);
    std::ostringstream out;
    out << "HTTP/1.1 " << resp.statusCode << " " << statusText(resp.statusCode) << "\r\n"
        << "Content-Type: " << resp.contentType << "\r\n"
        << "Content-Length: " << resp.body.size() << "\r\n"
        << "Access-Control-Allow-Origin: *\r\n"
        << "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n"
        << "Access-Control-Allow-Headers: Content-Type\r\n"
        << "Cache-Control: no-store\r\n"
        << "Connection: close\r\n\r\n"
        << resp.body;

    const std::string wire = out.str();
    sendAll(clientFd, wire.data(), wire.size());
}

HttpServer::HttpResponse HttpServer::routeRequest(const HttpRequest& req) {
    try {
        if (req.method == "OPTIONS") {
            return HttpResponse{200, "text/plain; charset=utf-8", ""};
        }
        if (req.method == "GET" && req.path == "/api/overview") {
            return handleOverview();
        }
        if (req.method == "POST" && req.path == "/api/array/generate") {
            return handleGenerateArray(req.body);
        }
        if (req.method == "POST" && req.path == "/api/sort") {
            return handleSort(req.body);
        }
        if (req.method == "POST" && req.path == "/api/search") {
            return handleSearch(req.body);
        }
        if (req.method == "GET" && req.path == "/api/ds/state") {
            return handleDsState();
        }
        if (req.method == "POST" && req.path == "/api/ds/stack") {
            return handleStackOp(req.body);
        }
        if (req.method == "POST" && req.path == "/api/ds/queue") {
            return handleQueueOp(req.body);
        }
        if (req.method == "POST" && req.path == "/api/ds/list") {
            return handleListOp(req.body);
        }
        if (req.method == "GET" && req.path == "/api/history") {
            return handleGetHistory();
        }
        if (req.method == "DELETE" && req.path == "/api/history") {
            return handleClearHistory();
        }
        if (req.method == "POST" && req.path == "/api/compare") {
            return handleCompare(req.body);
        }
        if (req.method == "GET" && req.path == "/api/bst/state") {
            return handleBstState();
        }
        if (req.method == "POST" && req.path == "/api/bst/op") {
            return handleBstOp(req.body);
        }
        if (req.method == "GET" && req.path == "/api/graph/state") {
            return handleGraphState();
        }
        if (req.method == "POST" && req.path == "/api/graph/op") {
            return handleGraphOp(req.body);
        }
        if (req.method == "POST" && req.path == "/api/quiz/start") {
            return handleQuizStart(req.body);
        }
        if (req.method == "POST" && req.path == "/api/quiz/answer") {
            return handleQuizAnswer(req.body);
        }
        if (req.method == "POST" && req.path == "/api/quiz/finish") {
            return handleQuizFinish(req.body);
        }
        if (req.method == "GET" && req.path == "/api/progress") {
            return handleGetProgress();
        }
        if (req.method == "POST" && req.path == "/api/progress/module") {
            return handleMarkModule(req.body);
        }
        if (req.method == "DELETE" && req.path == "/api/progress") {
            return handleResetProgress();
        }
        if (req.method == "GET" && req.path == "/api/code") {
            return handleGetCode();
        }
        if (req.method == "GET" && req.path == "/api/ecosystem") {
            return handleEcosystemState();
        }
        if (req.method == "POST" && req.path == "/api/preferences") {
            return handleSavePreferences(req.body);
        }
        if (req.method == "POST" && req.path == "/api/roadmap/toggle") {
            return handleToggleRoadmap(req.body);
        }
        if (req.method == "POST" && req.path == "/api/practice/update") {
            return handlePracticeUpdate(req.body);
        }
        if (req.method == "POST" && req.path == "/api/leetcode/add") {
            return handleLeetCodeAdd(req.body);
        }
        if (req.method == "POST" && req.path == "/api/leetcode/update") {
            return handleLeetCodeUpdate(req.body);
        }
        if (req.method == "POST" && req.path == "/api/leetcode/delete") {
            return handleLeetCodeDelete(req.body);
        }
        if (req.method == "POST" && req.path == "/api/revision/review") {
            return handleRevisionReview(req.body);
        }
        if (req.method == "POST" && req.path == "/api/revision/mistake") {
            return handleRevisionMistake(req.body);
        }
        if (req.method == "GET" && req.path == "/api/analytics") {
            return handleAnalytics();
        }
        if (req.method == "POST" && req.path == "/api/library/sync") {
            return handleLibSync();
        }
        if (req.method == "POST" && req.path == "/api/library/import") {
            return handleLibImport(req.body);
        }
        if (req.method == "POST" && req.path == "/api/library/query") {
            return handleLibQuery(req.body);
        }
        if (req.method == "POST" && req.path == "/api/library/detail") {
            return handleLibDetail(req.body);
        }
        if (req.method == "POST" && req.path == "/api/library/status") {
            return handleLibStatus(req.body);
        }
        if (req.method == "POST" && req.path == "/api/library/notes") {
            return handleLibNotes(req.body);
        }
        if (req.method == "POST" && req.path == "/api/notes/search") {
            return handleNotesSearch(req.body);
        }
        if (req.method == "POST" && req.path == "/api/solutions/add") {
            return handleSolutionsAdd(req.body);
        }
        if (req.method == "POST" && req.path == "/api/solutions/delete") {
            return handleSolutionsDelete(req.body);
        }
        if (req.method == "POST" && req.path == "/api/solutions/search") {
            return handleSolutionsSearch(req.body);
        }
        if (req.method == "GET" && req.path == "/api/topics") {
            return handleTopicsGet();
        }
        if (req.method == "POST" && req.path == "/api/topics/goal") {
            return handleTopicsGoal(req.body);
        }
        if (req.method == "POST" && req.path == "/api/leetcode/sync") {
            return handleLcSync(req.body);
        }
        if (req.method == "POST" && req.path == "/api/leetcode/import-submissions") {
            return handleLcImportSubmissions(req.body);
        }
        if (req.method == "GET" && req.path == "/api/leetcode/profile") {
            return handleLcProfileGet();
        }
        if (req.method == "POST" && req.path == "/api/extension/submission") {
            return handleExtensionSubmission(req.body);
        }
        if (req.method == "GET" && req.path == "/api/extension/status") {
            return handleExtensionStatus();
        }
        if (req.method == "GET" && req.path == "/api/intel/revision") {
            return handleIntelRevisionGet();
        }
        if (req.method == "POST" && req.path == "/api/intel/revision/review") {
            return handleIntelRevisionReview(req.body);
        }
        if (req.method == "POST" && req.path == "/api/intel/revision/schedule") {
            return handleIntelRevisionSchedule(req.body);
        }
        if (req.method == "GET" && req.path == "/api/intel/analytics") {
            return handleIntelAnalytics();
        }
        if (req.method == "GET" && req.path.rfind("/api/", 0) != 0) {
            return serveStaticFile(req.path);
        }
        if (req.path.rfind("/api/", 0) == 0) {
            return HttpResponse{404, "application/json; charset=utf-8",
                                "{\"ok\":false,\"code\":\"NOT_FOUND\",\"error\":\"Unknown API endpoint.\"}"};
        }
        return HttpResponse{405, "application/json; charset=utf-8",
                            "{\"ok\":false,\"code\":\"METHOD_NOT_ALLOWED\",\"error\":\"Method not allowed.\"}"};
    } catch (const DSAException& ex) {
        std::ostringstream oss;
        oss << "{\"ok\":false,\"code\":" << jsonEscape(ex.code())
            << ",\"error\":" << jsonEscape(ex.what()) << "}";
        return HttpResponse{400, "application/json; charset=utf-8", oss.str()};
    } catch (const std::exception& ex) {
        std::ostringstream oss;
        oss << "{\"ok\":false,\"code\":\"INTERNAL_ERROR\",\"error\":" << jsonEscape(ex.what()) << "}";
        return HttpResponse{500, "application/json; charset=utf-8", oss.str()};
    }
}

HttpServer::HttpResponse HttpServer::serveStaticFile(const std::string& requestPath) const {
    namespace fs = std::filesystem;

    std::string rel = (requestPath == "/" || requestPath.empty()) ? "index.html" : requestPath;
    while (!rel.empty() && rel.front() == '/') {
        rel.erase(rel.begin());
    }

    const fs::path root = fs::weakly_canonical(fs::path(webRoot_));
    const fs::path target = fs::weakly_canonical(root / rel);

    const std::string rootStr = root.string();
    const std::string targetStr = target.string();
    if (targetStr.compare(0, rootStr.size(), rootStr) != 0) {
        return HttpResponse{404, "text/plain; charset=utf-8", "Forbidden"};
    }

    if (!fs::exists(target) || !fs::is_regular_file(target)) {
        return HttpResponse{404, "text/plain; charset=utf-8", "File Not Found"};
    }

    std::ifstream file(target, std::ios::binary);
    if (!file.is_open()) {
        return HttpResponse{500, "text/plain; charset=utf-8", "Failed to read static file"};
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return HttpResponse{200, mimeTypeForPath(target), ss.str()};
}

// ============================================================================
// API Route Handlers
// ============================================================================

HttpServer::HttpResponse HttpServer::handleOverview() const {
    const auto infos = getAllAlgorithmInfo();
    const auto history = logger_.readRecent(10);

    std::ostringstream oss;
    oss << "{\"ok\":true,\"algorithms\":[";
    for (std::size_t i = 0; i < infos.size(); ++i) {
        if (i > 0) oss << ",";
        oss << serializeInfo(infos[i]);
    }
    oss << "],\"recentHistoryCount\":" << history.size() << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleGenerateArray(const std::string& body) const {
    JsonValue payload = JsonParser(body.empty() ? "{}" : body).parse();
    long long size = payload.getInt("size", 16);
    long long minVal = payload.getInt("min", 10);
    long long maxVal = payload.getInt("max", 99);

    if (size < 2 || size > 60) {
        throw ValidationException("Array size must be between 2 and 60.");
    }

    std::vector<int> arr = utils::generateRandomArray(
        static_cast<std::size_t>(size), static_cast<int>(minVal), static_cast<int>(maxVal));

    std::ostringstream oss;
    oss << "{\"ok\":true,\"array\":" << jsonIntArray(arr) << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleSort(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string algo = payload.getString("algorithm");
    std::vector<int> arr = payload.getIntArray("array", true);

    auto sorter = createSorter(algo);
    SortResult res = sorter->sort(arr);

    logger_.log("Sorting", res.info.name,
                "n=" + std::to_string(res.initialArray.size()) +
                    ", comparisons=" + std::to_string(res.metrics.comparisons) +
                    ", swaps=" + std::to_string(res.metrics.swaps));
    progress_.markModule("sorting", true);

    std::ostringstream oss;
    oss << "{\"ok\":true,"
        << "\"info\":" << serializeInfo(res.info) << ","
        << "\"initialArray\":" << jsonIntArray(res.initialArray) << ","
        << "\"sortedArray\":" << jsonIntArray(res.sortedArray) << ","
        << "\"comparisons\":" << res.metrics.comparisons << ","
        << "\"swaps\":" << res.metrics.swaps << ","
        << "\"steps\":[";

    for (std::size_t i = 0; i < res.steps.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& st = res.steps[i];
        oss << "{"
            << "\"array\":" << jsonIntArray(st.array) << ","
            << "\"indexA\":" << st.indexA << ","
            << "\"indexB\":" << st.indexB << ","
            << "\"sortedIndices\":" << jsonIntArray(st.sortedIndices) << ","
            << "\"action\":" << jsonEscape(st.action) << ","
            << "\"detail\":" << jsonEscape(st.detail) << ","
            << "\"comparisons\":" << st.comparisons << ","
            << "\"swaps\":" << st.swaps
            << "}";
    }
    oss << "]}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleSearch(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string algo = payload.getString("algorithm");
    std::vector<int> arr = payload.getIntArray("array", true);
    long long targetLl = payload.getInt("target", 0, true);
    bool autoSort = payload.getBool("autoSort", false);

    if (targetLl < -99999 || targetLl > 99999) {
        throw ValidationException("Search target must be between -99999 and 99999.");
    }
    int target = static_cast<int>(targetLl);

    if (algo == "binary" && autoSort && !utils::isSorted(arr)) {
        auto insertion = createSorter("insertion");
        arr = insertion->sort(arr).sortedArray;
    }

    auto searcher = createSearcher(algo);
    SearchResult res = searcher->search(arr, target);

    logger_.log("Searching", res.info.name,
                "target=" + std::to_string(target) +
                    ", found=" + (res.found ? ("index " + std::to_string(res.foundIndex)) : "no") +
                    ", comparisons=" + std::to_string(res.comparisons));
    progress_.markModule("searching", true);

    std::ostringstream oss;
    oss << "{\"ok\":true,"
        << "\"info\":" << serializeInfo(res.info) << ","
        << "\"arrayUsed\":" << jsonIntArray(res.arrayUsed) << ","
        << "\"target\":" << res.target << ","
        << "\"found\":" << (res.found ? "true" : "false") << ","
        << "\"foundIndex\":" << res.foundIndex << ","
        << "\"comparisons\":" << res.comparisons << ","
        << "\"steps\":[";

    for (std::size_t i = 0; i < res.steps.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& st = res.steps[i];
        oss << "{"
            << "\"index\":" << st.index << ","
            << "\"low\":" << st.low << ","
            << "\"high\":" << st.high << ","
            << "\"mid\":" << st.mid << ","
            << "\"value\":" << st.value << ","
            << "\"status\":" << jsonEscape(st.status) << ","
            << "\"detail\":" << jsonEscape(st.detail) << ","
            << "\"comparisons\":" << st.comparisons
            << "}";
    }
    oss << "]}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleDsState() {
    std::lock_guard<std::mutex> lock(dsMutex_);
    std::ostringstream oss;
    oss << "{\"ok\":true,"
        << "\"stack\":" << serializeDsState(stack_.state()) << ","
        << "\"queue\":" << serializeDsState(queue_.state()) << ","
        << "\"list\":" << serializeDsState(linkedList_.state())
        << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleStackOp(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string op = payload.getString("op");

    std::lock_guard<std::mutex> lock(dsMutex_);
    std::string message;
    bool hasOutputValue = false;
    int outputValue = 0;

    if (op == "push") {
        long long val = payload.getInt("value", 0, true);
        if (val < -99999 || val > 99999) {
            throw ValidationException("Value must be between -99999 and 99999.");
        }
        stack_.push(static_cast<int>(val));
        message = "Pushed " + std::to_string(val) + " onto Stack.";
        logger_.log("Stack", "Push", message);
    } else if (op == "pop") {
        outputValue = stack_.pop();
        hasOutputValue = true;
        message = "Popped " + std::to_string(outputValue) + " from Stack.";
        logger_.log("Stack", "Pop", message);
    } else if (op == "peek") {
        outputValue = stack_.peek();
        hasOutputValue = true;
        message = "Top element is " + std::to_string(outputValue) + ".";
        logger_.log("Stack", "Peek", message);
    } else if (op == "clear") {
        stack_.clear();
        message = "Cleared all elements from Stack.";
        logger_.log("Stack", "Clear", message);
    } else if (op == "set_capacity") {
        long long cap = payload.getInt("capacity", 8, true);
        if (cap < 2 || cap > 16) {
            throw ValidationException("Capacity must be between 2 and 16.");
        }
        stack_.setCapacity(static_cast<std::size_t>(cap));
        message = "Stack capacity updated to " + std::to_string(cap) + ".";
        logger_.log("Stack", "Capacity", message);
    } else {
        throw ValidationException("Unsupported Stack operation: '" + op + "'.");
    }

    std::ostringstream oss;
    oss << "{\"ok\":true,"
        << "\"message\":" << jsonEscape(message) << ","
        << "\"hasOutputValue\":" << (hasOutputValue ? "true" : "false") << ","
        << "\"outputValue\":" << outputValue << ","
        << "\"stack\":" << serializeDsState(stack_.state())
        << "}";
    progress_.markModule("datastructures", true);
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleQueueOp(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string op = payload.getString("op");

    std::lock_guard<std::mutex> lock(dsMutex_);
    std::string message;
    bool hasOutputValue = false;
    int outputValue = 0;

    if (op == "enqueue") {
        long long val = payload.getInt("value", 0, true);
        if (val < -99999 || val > 99999) {
            throw ValidationException("Value must be between -99999 and 99999.");
        }
        queue_.enqueue(static_cast<int>(val));
        message = "Enqueued " + std::to_string(val) + " at rear of Queue.";
        logger_.log("Queue", "Enqueue", message);
    } else if (op == "dequeue") {
        outputValue = queue_.dequeue();
        hasOutputValue = true;
        message = "Dequeued " + std::to_string(outputValue) + " from front of Queue.";
        logger_.log("Queue", "Dequeue", message);
    } else if (op == "front") {
        outputValue = queue_.front();
        hasOutputValue = true;
        message = "Front element is " + std::to_string(outputValue) + ".";
        logger_.log("Queue", "Front", message);
    } else if (op == "clear") {
        queue_.clear();
        message = "Cleared all elements from Queue.";
        logger_.log("Queue", "Clear", message);
    } else if (op == "set_capacity") {
        long long cap = payload.getInt("capacity", 8, true);
        if (cap < 2 || cap > 16) {
            throw ValidationException("Capacity must be between 2 and 16.");
        }
        queue_.setCapacity(static_cast<std::size_t>(cap));
        message = "Queue capacity updated to " + std::to_string(cap) + ".";
        logger_.log("Queue", "Capacity", message);
    } else {
        throw ValidationException("Unsupported Queue operation: '" + op + "'.");
    }

    std::ostringstream oss;
    oss << "{\"ok\":true,"
        << "\"message\":" << jsonEscape(message) << ","
        << "\"hasOutputValue\":" << (hasOutputValue ? "true" : "false") << ","
        << "\"outputValue\":" << outputValue << ","
        << "\"queue\":" << serializeDsState(queue_.state())
        << "}";
    progress_.markModule("datastructures", true);
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleListOp(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string op = payload.getString("op");

    std::lock_guard<std::mutex> lock(dsMutex_);
    std::string message;
    int highlightIndex = -1;

    if (op == "insert_head") {
        long long val = payload.getInt("value", 0, true);
        linkedList_.insertHead(static_cast<int>(val));
        highlightIndex = 0;
        message = "Inserted " + std::to_string(val) + " at HEAD.";
        logger_.log("Linked List", "Insert Head", message);
    } else if (op == "insert_tail") {
        long long val = payload.getInt("value", 0, true);
        linkedList_.insertTail(static_cast<int>(val));
        highlightIndex = static_cast<int>(linkedList_.size()) - 1;
        message = "Inserted " + std::to_string(val) + " at TAIL.";
        logger_.log("Linked List", "Insert Tail", message);
    } else if (op == "remove_head") {
        int val = linkedList_.removeHead();
        message = "Removed HEAD node (" + std::to_string(val) + ").";
        logger_.log("Linked List", "Remove Head", message);
    } else if (op == "remove_value") {
        long long val = payload.getInt("value", 0, true);
        bool removed = linkedList_.removeValue(static_cast<int>(val));
        if (!removed) {
            throw ValidationException("Value " + std::to_string(val) + " not found in Linked List.");
        }
        message = "Deleted node " + std::to_string(val) + " from Linked List.";
        logger_.log("Linked List", "Delete", message);
    } else if (op == "search") {
        long long val = payload.getInt("value", 0, true);
        highlightIndex = linkedList_.findIndex(static_cast<int>(val));
        message = highlightIndex >= 0
            ? ("Found " + std::to_string(val) + " at node index " + std::to_string(highlightIndex) + ".")
            : ("Value " + std::to_string(val) + " was not found in Linked List.");
        logger_.log("Linked List", "Search", message);
    } else if (op == "sample") {
        linkedList_.loadSample();
        message = "Loaded sample Linked List (12 -> 27 -> 44 -> 68).";
        logger_.log("Linked List", "Sample", message);
    } else if (op == "clear") {
        linkedList_.clear();
        message = "Cleared all nodes from Linked List.";
        logger_.log("Linked List", "Clear", message);
    } else {
        throw ValidationException("Unsupported Linked List operation: '" + op + "'.");
    }

    progress_.markModule("datastructures", true);
    std::ostringstream oss;
    oss << "{\"ok\":true,"
        << "\"message\":" << jsonEscape(message) << ","
        << "\"highlightIndex\":" << highlightIndex << ","
        << "\"list\":" << serializeDsState(linkedList_.state())
        << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleGetHistory() const {
    const auto records = logger_.readRecent(25);
    std::ostringstream oss;
    oss << "{\"ok\":true,\"records\":[";
    for (std::size_t i = 0; i < records.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{"
            << "\"timestamp\":" << jsonEscape(records[i].timestamp) << ","
            << "\"category\":" << jsonEscape(records[i].category) << ","
            << "\"operation\":" << jsonEscape(records[i].operation) << ","
            << "\"summary\":" << jsonEscape(records[i].summary)
            << "}";
    }
    oss << "]}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleClearHistory() {
    logger_.clear();
    return HttpResponse{200, "application/json; charset=utf-8",
                        "{\"ok\":true,\"message\":\"History log cleared.\"}"};
}

// ============================================================================
// Upgraded Module Handlers
// ============================================================================

HttpServer::HttpResponse HttpServer::handleCompare(const std::string& body) {
    JsonValue payload = JsonParser(body.empty() ? "{}" : body).parse();
    std::vector<int> arr;
    std::string patternUsed = "custom";

    if (payload.has("array")) {
        arr = payload.getIntArray("array", true);
    } else {
        long long size = payload.getInt("size", 200);
        patternUsed = payload.getString("pattern", "random");
        if (size < 2 || size > 2000) {
            throw ValidationException("Benchmark array size must be between 2 and 2000.");
        }
        arr = utils::generatePatternArray(static_cast<std::size_t>(size), patternUsed);
    }

    const auto results = compareSortingAlgorithms(arr);
    progress_.markModule("comparator", true);
    logger_.log("Comparator", "Benchmark",
                "n=" + std::to_string(arr.size()) + ", pattern=" + patternUsed);

    std::ostringstream oss;
    oss << std::fixed;
    oss.precision(3);
    oss << "{\"ok\":true,"
        << "\"inputSize\":" << arr.size() << ","
        << "\"pattern\":" << jsonEscape(patternUsed) << ","
        << "\"results\":[";
    for (std::size_t i = 0; i < results.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& r = results[i];
        oss << "{"
            << "\"info\":" << serializeInfo(r.info) << ","
            << "\"inputSize\":" << r.inputSize << ","
            << "\"comparisons\":" << r.comparisons << ","
            << "\"swaps\":" << r.swaps << ","
            << "\"elapsedMicroseconds\":" << r.elapsedMicroseconds << ","
            << "\"elapsedNanoseconds\":" << r.elapsedNanoseconds
            << "}";
    }
    oss << "]}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleBstState() {
    std::lock_guard<std::mutex> lock(dsMutex_);
    return HttpResponse{200, "application/json; charset=utf-8", serializeBstResult(bst_.snapshot())};
}

HttpServer::HttpResponse HttpServer::handleBstOp(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string op = payload.getString("op");

    std::lock_guard<std::mutex> lock(dsMutex_);
    BstOperationResult res;

    if (op == "insert") {
        long long val = payload.getInt("value", 0, true);
        res = bst_.insert(static_cast<int>(val));
    } else if (op == "search") {
        long long val = payload.getInt("value", 0, true);
        res = bst_.search(static_cast<int>(val));
    } else if (op == "delete") {
        long long val = payload.getInt("value", 0, true);
        res = bst_.remove(static_cast<int>(val));
    } else if (op == "traverse") {
        std::string order = payload.getString("order", "inorder");
        res = bst_.traverse(order);
    } else if (op == "sample") {
        res = bst_.loadSample();
    } else if (op == "clear") {
        res = bst_.clear();
    } else {
        throw ValidationException("Unsupported BST operation: '" + op + "'.");
    }

    progress_.markModule("bst", true);
    logger_.log("BST", op, res.message);
    return HttpResponse{200, "application/json; charset=utf-8", serializeBstResult(res)};
}

HttpServer::HttpResponse HttpServer::handleGraphState() {
    std::lock_guard<std::mutex> lock(dsMutex_);
    std::ostringstream oss;
    oss << "{\"ok\":true,\"graph\":" << serializeGraphSnapshot(graph_.snapshot()) << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleGraphOp(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string op = payload.getString("op");

    std::lock_guard<std::mutex> lock(dsMutex_);
    std::string message;
    bool hasTraversal = false;
    GraphTraversalResult trav;

    if (op == "add_vertex") {
        std::string v = payload.getString("vertex");
        graph_.addVertex(v);
        message = "Added vertex.";
    } else if (op == "remove_vertex") {
        std::string v = payload.getString("vertex");
        graph_.removeVertex(v);
        message = "Removed vertex.";
    } else if (op == "add_edge") {
        graph_.addEdge(payload.getString("from"), payload.getString("to"));
        message = "Added edge.";
    } else if (op == "remove_edge") {
        graph_.removeEdge(payload.getString("from"), payload.getString("to"));
        message = "Removed edge.";
    } else if (op == "set_directed") {
        bool dir = payload.getBool("directed", false);
        graph_.setDirected(dir);
        message = dir ? "Switched to Directed Graph." : "Switched to Undirected Graph.";
    } else if (op == "sample") {
        graph_.loadSample();
        message = "Loaded 7-vertex sample graph.";
    } else if (op == "clear") {
        graph_.clear();
        message = "Cleared graph.";
    } else if (op == "bfs") {
        trav = graph_.bfs(payload.getString("start"));
        hasTraversal = true;
        message = "BFS traversal from " + trav.startVertex + " complete.";
    } else if (op == "dfs") {
        trav = graph_.dfs(payload.getString("start"));
        hasTraversal = true;
        message = "DFS traversal from " + trav.startVertex + " complete.";
    } else {
        throw ValidationException("Unsupported Graph operation: '" + op + "'.");
    }

    progress_.markModule("graph", true);
    logger_.log("Graph", op, message);

    std::ostringstream oss;
    oss << "{\"ok\":true,"
        << "\"message\":" << jsonEscape(message) << ","
        << "\"graph\":" << serializeGraphSnapshot(graph_.snapshot());

    if (hasTraversal) {
        oss << ",\"traversal\":{"
            << "\"algorithm\":" << jsonEscape(trav.algorithm) << ","
            << "\"startVertex\":" << jsonEscape(trav.startVertex) << ","
            << "\"order\":" << jsonStringArray(trav.order) << ","
            << "\"steps\":[";
        for (std::size_t i = 0; i < trav.steps.size(); ++i) {
            if (i > 0) oss << ",";
            const auto& st = trav.steps[i];
            oss << "{"
                << "\"current\":" << jsonEscape(st.current) << ","
                << "\"edgeFrom\":" << jsonEscape(st.edgeFrom) << ","
                << "\"edgeTo\":" << jsonEscape(st.edgeTo) << ","
                << "\"visited\":" << jsonStringArray(st.visited) << ","
                << "\"frontier\":" << jsonStringArray(st.frontier) << ","
                << "\"action\":" << jsonEscape(st.action) << ","
                << "\"detail\":" << jsonEscape(st.detail)
                << "}";
        }
        oss << "]}";
    }
    oss << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleQuizStart(const std::string& body) {
    JsonValue payload = JsonParser(body.empty() ? "{}" : body).parse();
    long long count = payload.getInt("count", 8);
    std::string category = payload.getString("category", "all");
    if (count < 1 || count > 15) {
        throw ValidationException("Quiz question count must be between 1 and 15.");
    }

    auto res = quiz_.startSession(static_cast<std::size_t>(count), category);
    std::ostringstream oss;
    oss << "{\"ok\":true,"
        << "\"sessionId\":" << jsonEscape(res.sessionId) << ","
        << "\"questions\":[";
    for (std::size_t i = 0; i < res.questions.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& q = res.questions[i];
        oss << "{"
            << "\"id\":" << q.id << ","
            << "\"category\":" << jsonEscape(q.category) << ","
            << "\"question\":" << jsonEscape(q.question) << ","
            << "\"options\":" << jsonStringArray(q.options)
            << "}";
    }
    oss << "]}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleQuizAnswer(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string sid = payload.getString("sessionId");
    int qid = static_cast<int>(payload.getInt("questionId", 0, true));
    int selected = static_cast<int>(payload.getInt("selectedIndex", 0, true));

    auto ans = quiz_.submitAnswer(sid, qid, selected);
    std::ostringstream oss;
    oss << "{\"ok\":true,"
        << "\"questionId\":" << ans.questionId << ","
        << "\"correct\":" << (ans.correct ? "true" : "false") << ","
        << "\"selectedIndex\":" << ans.selectedIndex << ","
        << "\"correctIndex\":" << ans.correctIndex << ","
        << "\"explanation\":" << jsonEscape(ans.explanation) << ","
        << "\"currentScore\":" << ans.currentScore << ","
        << "\"answeredCount\":" << ans.answeredCount << ","
        << "\"totalQuestions\":" << ans.totalQuestions
        << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleQuizFinish(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string sid = payload.getString("sessionId");

    QuizRecord rec = quiz_.finishSession(sid);
    progress_.recordQuiz(rec);
    hub_.recordActivityToday(1);
    logger_.log("Quiz", rec.category,
                "score=" + std::to_string(rec.score) + "/" + std::to_string(rec.total));

    std::ostringstream oss;
    oss << std::fixed;
    oss.precision(1);
    oss << "{\"ok\":true,"
        << "\"record\":{"
        << "\"timestamp\":" << jsonEscape(rec.timestamp) << ","
        << "\"category\":" << jsonEscape(rec.category) << ","
        << "\"score\":" << rec.score << ","
        << "\"total\":" << rec.total << ","
        << "\"accuracy\":" << rec.accuracy << "},"
        << "\"progress\":" << serializeProgress(progress_.snapshot())
        << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleGetProgress() const {
    std::ostringstream oss;
    oss << "{\"ok\":true,\"progress\":" << serializeProgress(progress_.snapshot()) << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleMarkModule(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string moduleId = payload.getString("module");
    bool completed = payload.getBool("completed", true);
    progress_.markModule(moduleId, completed);

    std::ostringstream oss;
    oss << "{\"ok\":true,\"progress\":" << serializeProgress(progress_.snapshot()) << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleResetProgress() {
    progress_.reset();
    logger_.log("Progress", "Reset", "Reset learning progress.");
    std::ostringstream oss;
    oss << "{\"ok\":true,\"progress\":" << serializeProgress(progress_.snapshot()) << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleGetCode() {
    progress_.markModule("codeviewer", true);
    const auto entries = getAllAlgorithmCodeEntries();
    std::ostringstream oss;
    oss << "{\"ok\":true,\"entries\":[";
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& e = entries[i];
        oss << "{"
            << "\"id\":" << jsonEscape(e.id) << ","
            << "\"name\":" << jsonEscape(e.name) << ","
            << "\"category\":" << jsonEscape(e.category) << ","
            << "\"bestTime\":" << jsonEscape(e.bestTime) << ","
            << "\"averageTime\":" << jsonEscape(e.averageTime) << ","
            << "\"worstTime\":" << jsonEscape(e.worstTime) << ","
            << "\"spaceComplexity\":" << jsonEscape(e.spaceComplexity) << ","
            << "\"cppCode\":" << jsonEscape(e.cppCode) << ","
            << "\"pseudocode\":" << jsonEscape(e.pseudocode)
            << "}";
    }
    oss << "]}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

// ============================================================================
// Ecosystem Handlers (Roadmap, Practice, LeetCode, Revision, Analytics)
// ============================================================================

namespace {

std::string serializeAnalytics(const AnalyticsSnapshot& a) {
    std::ostringstream oss;
    oss << std::fixed;
    oss.precision(1);
    oss << "{"
        << "\"problemsSolved\":" << a.problemsSolved << ","
        << "\"problemsAttempted\":" << a.problemsAttempted << ","
        << "\"leetcodeSolved\":" << a.leetcodeSolved << ","
        << "\"leetcodeTotal\":" << a.leetcodeTotal << ","
        << "\"roadmapCompleted\":" << a.roadmapCompleted << ","
        << "\"roadmapTotal\":" << a.roadmapTotal << ","
        << "\"topicCompletionPercent\":" << a.topicCompletionPercent << ","
        << "\"quizAttempts\":" << a.quizAttempts << ","
        << "\"quizAvgAccuracy\":" << a.quizAvgAccuracy << ","
        << "\"revisionTotalReviews\":" << a.revisionTotalReviews << ","
        << "\"revisionDueCount\":" << a.revisionDueCount << ","
        << "\"needsRevisionCount\":" << a.needsRevisionCount << ","
        << "\"mistakesCount\":" << a.mistakesCount << ","
        << "\"weeklyActivity\":[";
    for (std::size_t i = 0; i < a.weeklyActivity.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{\"date\":" << jsonEscape(a.weeklyActivity[i].date)
            << ",\"count\":" << a.weeklyActivity[i].count << "}";
    }
    oss << "],\"dailyHeatmap\":[";
    for (std::size_t i = 0; i < a.dailyHeatmap.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{\"date\":" << jsonEscape(a.dailyHeatmap[i].date)
            << ",\"count\":" << a.dailyHeatmap[i].count << "}";
    }
    oss << "],\"mostPracticedTopics\":[";
    for (std::size_t i = 0; i < a.mostPracticedTopics.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{\"topic\":" << jsonEscape(a.mostPracticedTopics[i].topic)
            << ",\"count\":" << a.mostPracticedTopics[i].count << "}";
    }
    oss << "]}";
    return oss.str();
}

std::string serializeEcosystemPayload(const LearningHub& hub, const ProgressManager& progress,
                                      const HistoryLogger& logger) {
    const auto prefs = hub.preferences();
    const auto roadmap = hub.roadmap();
    const auto practice = hub.practiceProblems();
    const auto leetcode = hub.leetcodeProblems();
    const auto cards = hub.flashcards();
    const auto mistakes = hub.mistakes();
    const auto resources = hub.resources();
    const auto analytics = hub.computeAnalytics(progress, logger);

    std::ostringstream oss;
    oss << std::fixed;
    oss.precision(2);
    oss << "{\"ok\":true,"
        << "\"preferences\":{\"leetcodeUsername\":" << jsonEscape(prefs.leetcodeUsername)
        << ",\"learningGoal\":" << jsonEscape(prefs.learningGoal) << "},"
        << "\"roadmap\":[";
    for (std::size_t i = 0; i < roadmap.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& t = roadmap[i];
        oss << "{"
            << "\"id\":" << jsonEscape(t.id) << ","
            << "\"stage\":" << jsonEscape(t.stage) << ","
            << "\"title\":" << jsonEscape(t.title) << ","
            << "\"explanation\":" << jsonEscape(t.explanation) << ","
            << "\"difficulty\":" << jsonEscape(t.difficulty) << ","
            << "\"prerequisites\":" << jsonStringArray(t.prerequisites) << ","
            << "\"relatedVisualizer\":" << jsonEscape(t.relatedVisualizer) << ","
            << "\"relatedProblemIds\":" << jsonStringArray(t.relatedProblemIds) << ","
            << "\"completed\":" << (t.completed ? "true" : "false")
            << "}";
    }
    oss << "],\"practice\":[";
    for (std::size_t i = 0; i < practice.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& p = practice[i];
        oss << "{"
            << "\"id\":" << jsonEscape(p.id) << ","
            << "\"title\":" << jsonEscape(p.title) << ","
            << "\"topic\":" << jsonEscape(p.topic) << ","
            << "\"difficulty\":" << jsonEscape(p.difficulty) << ","
            << "\"prompt\":" << jsonEscape(p.prompt) << ","
            << "\"hint\":" << jsonEscape(p.hint) << ","
            << "\"defaultApproach\":" << jsonEscape(p.defaultApproach) << ","
            << "\"solved\":" << (p.solved ? "true" : "false") << ","
            << "\"bookmarked\":" << (p.bookmarked ? "true" : "false") << ","
            << "\"needsRevision\":" << (p.needsRevision ? "true" : "false") << ","
            << "\"attempts\":" << p.attempts << ","
            << "\"notes\":" << jsonEscape(p.notes) << ","
            << "\"userApproach\":" << jsonEscape(p.userApproach)
            << "}";
    }
    oss << "],\"leetcode\":[";
    for (std::size_t i = 0; i < leetcode.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& l = leetcode[i];
        oss << "{"
            << "\"id\":" << jsonEscape(l.id) << ","
            << "\"title\":" << jsonEscape(l.title) << ","
            << "\"url\":" << jsonEscape(l.url) << ","
            << "\"difficulty\":" << jsonEscape(l.difficulty) << ","
            << "\"topic\":" << jsonEscape(l.topic) << ","
            << "\"status\":" << jsonEscape(l.status) << ","
            << "\"bookmarked\":" << (l.bookmarked ? "true" : "false") << ","
            << "\"notes\":" << jsonEscape(l.notes) << ","
            << "\"updatedAt\":" << jsonEscape(l.updatedAt)
            << "}";
    }
    oss << "],\"flashcards\":[";
    for (std::size_t i = 0; i < cards.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& c = cards[i];
        oss << "{"
            << "\"id\":" << jsonEscape(c.id) << ","
            << "\"category\":" << jsonEscape(c.category) << ","
            << "\"front\":" << jsonEscape(c.front) << ","
            << "\"back\":" << jsonEscape(c.back) << ","
            << "\"intervalDays\":" << c.intervalDays << ","
            << "\"easeFactor\":" << c.easeFactor << ","
            << "\"repetitions\":" << c.repetitions << ","
            << "\"nextReviewDate\":" << jsonEscape(c.nextReviewDate) << ","
            << "\"lastRating\":" << jsonEscape(c.lastRating) << ","
            << "\"reviewCount\":" << c.reviewCount
            << "}";
    }
    oss << "],\"mistakes\":[";
    for (std::size_t i = 0; i < mistakes.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& m = mistakes[i];
        oss << "{"
            << "\"id\":" << jsonEscape(m.id) << ","
            << "\"title\":" << jsonEscape(m.title) << ","
            << "\"topic\":" << jsonEscape(m.topic) << ","
            << "\"note\":" << jsonEscape(m.note) << ","
            << "\"createdAt\":" << jsonEscape(m.createdAt)
            << "}";
    }
    oss << "],\"resources\":[";
    for (std::size_t i = 0; i < resources.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& r = resources[i];
        oss << "{"
            << "\"id\":" << jsonEscape(r.id) << ","
            << "\"name\":" << jsonEscape(r.name) << ","
            << "\"category\":" << jsonEscape(r.category) << ","
            << "\"description\":" << jsonEscape(r.description) << ","
            << "\"url\":" << jsonEscape(r.url)
            << "}";
    }
    oss << "],\"analytics\":" << serializeAnalytics(analytics) << "}";
    return oss.str();
}

} // namespace

HttpServer::HttpResponse HttpServer::handleEcosystemState() const {
    return HttpResponse{200, "application/json; charset=utf-8",
                        serializeEcosystemPayload(hub_, progress_, logger_)};
}

HttpServer::HttpResponse HttpServer::handleSavePreferences(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string username = payload.has("leetcodeUsername")
        ? payload.getString("leetcodeUsername")
        : hub_.preferences().leetcodeUsername;
    std::string goal = payload.has("learningGoal")
        ? payload.getString("learningGoal")
        : hub_.preferences().learningGoal;
    hub_.setPreferences(username, goal);
    logger_.log("Preferences", "Update", "Updated preferences");
    return handleEcosystemState();
}

HttpServer::HttpResponse HttpServer::handleToggleRoadmap(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string topicId = payload.getString("topicId");
    bool completed = payload.getBool("completed", true);
    hub_.toggleRoadmapTopic(topicId, completed);
    logger_.log("Roadmap", topicId, completed ? "Marked completed" : "Marked incomplete");
    return handleEcosystemState();
}

HttpServer::HttpResponse HttpServer::handlePracticeUpdate(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string pid = payload.getString("problemId");
    std::string action = payload.getString("action");
    std::string notes = payload.getString("notes", "");
    std::string approach = payload.getString("approach", "");
    hub_.updatePracticeProblem(pid, action, notes, approach);
    logger_.log("Practice", pid, action);
    return handleEcosystemState();
}

HttpServer::HttpResponse HttpServer::handleLeetCodeAdd(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    auto lp = hub_.addLeetCodeProblem(
        payload.getString("title"),
        payload.getString("url", ""),
        payload.getString("difficulty", "Medium"),
        payload.getString("topic", "Arrays"),
        payload.getString("status", "Not Started"),
        payload.getString("notes", ""));
    logger_.log("LeetCode", "Add", lp.title);
    return handleEcosystemState();
}

HttpServer::HttpResponse HttpServer::handleLeetCodeUpdate(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string id = payload.getString("id");
    bool hasBookmark = payload.has("bookmarked");
    bool bookmarkVal = hasBookmark ? payload.getBool("bookmarked", false) : false;
    bool toggleBookmark = payload.getBool("toggleBookmark", false);
    bool hasNotes = payload.has("notes");
    std::string notes = hasNotes ? payload.getString("notes", "") : "";
    auto lp = hub_.updateLeetCodeProblem(
        id,
        payload.getString("status", ""),
        toggleBookmark,
        hasBookmark,
        bookmarkVal,
        hasNotes,
        notes,
        payload.getString("difficulty", ""),
        payload.getString("topic", ""));
    logger_.log("LeetCode", "Update", lp.title + " -> " + lp.status);
    return handleEcosystemState();
}

HttpServer::HttpResponse HttpServer::handleLeetCodeDelete(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string id = payload.getString("id");
    hub_.deleteLeetCodeProblem(id);
    logger_.log("LeetCode", "Delete", id);
    return handleEcosystemState();
}

HttpServer::HttpResponse HttpServer::handleRevisionReview(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string cardId = payload.getString("cardId");
    std::string rating = payload.getString("rating");
    auto card = hub_.reviewFlashcard(cardId, rating);
    logger_.log("Revision", card.category, "Reviewed " + card.id + " (" + card.lastRating + ")");
    return handleEcosystemState();
}

HttpServer::HttpResponse HttpServer::handleRevisionMistake(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string op = payload.getString("op");
    if (op == "add") {
        auto m = hub_.addMistake(
            payload.getString("title"),
            payload.getString("topic", "General"),
            payload.getString("note"));
        logger_.log("Revision", "Mistake Added", m.title);
    } else if (op == "remove") {
        hub_.removeMistake(payload.getString("id"));
        logger_.log("Revision", "Mistake Removed", payload.getString("id"));
    } else {
        throw ValidationException("Unsupported mistake op: '" + op + "'.");
    }
    return handleEcosystemState();
}

HttpServer::HttpResponse HttpServer::handleAnalytics() const {
    std::ostringstream oss;
    oss << "{\"ok\":true,\"analytics\":"
        << serializeAnalytics(hub_.computeAnalytics(progress_, logger_))
        << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

// ============================================================================
// Intelligence Hub Serialization & Handlers
// ============================================================================

namespace {

std::string serializeCatalogProblem(const CatalogProblem& p) {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":" << p.id << ","
        << "\"title\":" << jsonEscape(p.title) << ","
        << "\"slug\":" << jsonEscape(p.slug) << ","
        << "\"difficulty\":" << jsonEscape(p.difficulty) << ","
        << "\"topicTags\":" << jsonStringArray(p.topicTags) << ","
        << "\"url\":" << jsonEscape(p.url) << ","
        << "\"isPremium\":" << (p.isPremium ? "true" : "false")
        << "}";
    return oss.str();
}

std::string serializeProblemProgress(const ProblemProgressRecord& r) {
    std::ostringstream oss;
    oss << std::fixed;
    oss.precision(2);
    oss << "{"
        << "\"slug\":" << jsonEscape(r.slug) << ","
        << "\"status\":" << jsonEscape(r.status) << ","
        << "\"firstAttemptDate\":" << jsonEscape(r.firstAttemptDate) << ","
        << "\"latestAttemptDate\":" << jsonEscape(r.latestAttemptDate) << ","
        << "\"attemptCount\":" << r.attemptCount << ","
        << "\"accepted\":" << (r.accepted ? "true" : "false") << ","
        << "\"source\":" << jsonEscape(r.source) << ","
        << "\"bookmarked\":" << (r.bookmarked ? "true" : "false") << ","
        << "\"needsRevision\":" << (r.needsRevision ? "true" : "false") << ","
        << "\"revisionIntervalDays\":" << r.revisionIntervalDays << ","
        << "\"revisionEase\":" << r.revisionEase << ","
        << "\"revisionReps\":" << r.revisionReps << ","
        << "\"nextReviewDate\":" << jsonEscape(r.nextReviewDate) << ","
        << "\"lastRating\":" << jsonEscape(r.lastRating) << ","
        << "\"reviewCount\":" << r.reviewCount
        << "}";
    return oss.str();
}

std::string serializeNotes(const ProblemNotes& n) {
    std::ostringstream oss;
    oss << "{"
        << "\"slug\":" << jsonEscape(n.slug) << ","
        << "\"intuition\":" << jsonEscape(n.intuition) << ","
        << "\"bruteForce\":" << jsonEscape(n.bruteForce) << ","
        << "\"optimized\":" << jsonEscape(n.optimized) << ","
        << "\"timeComplexity\":" << jsonEscape(n.timeComplexity) << ","
        << "\"spaceComplexity\":" << jsonEscape(n.spaceComplexity) << ","
        << "\"mistakesMade\":" << jsonEscape(n.mistakesMade) << ","
        << "\"observations\":" << jsonEscape(n.observations) << ","
        << "\"cppSnippet\":" << jsonEscape(n.cppSnippet) << ","
        << "\"revisionComments\":" << jsonEscape(n.revisionComments) << ","
        << "\"updatedAt\":" << jsonEscape(n.updatedAt)
        << "}";
    return oss.str();
}

std::string serializeSolution(const SavedSolution& s) {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":" << jsonEscape(s.id) << ","
        << "\"slug\":" << jsonEscape(s.slug) << ","
        << "\"problemId\":" << s.problemId << ","
        << "\"problemTitle\":" << jsonEscape(s.problemTitle) << ","
        << "\"approachName\":" << jsonEscape(s.approachName) << ","
        << "\"timeComplexity\":" << jsonEscape(s.timeComplexity) << ","
        << "\"spaceComplexity\":" << jsonEscape(s.spaceComplexity) << ","
        << "\"explanation\":" << jsonEscape(s.explanation) << ","
        << "\"cppCode\":" << jsonEscape(s.cppCode) << ","
        << "\"topics\":" << jsonStringArray(s.topics) << ","
        << "\"createdAt\":" << jsonEscape(s.createdAt)
        << "}";
    return oss.str();
}

std::string serializeSubmission(const SubmissionRecord& sub) {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":" << jsonEscape(sub.id) << ","
        << "\"slug\":" << jsonEscape(sub.slug) << ","
        << "\"problemId\":" << sub.problemId << ","
        << "\"title\":" << jsonEscape(sub.title) << ","
        << "\"difficulty\":" << jsonEscape(sub.difficulty) << ","
        << "\"timestamp\":" << jsonEscape(sub.timestamp) << ","
        << "\"date\":" << jsonEscape(sub.date) << ","
        << "\"source\":" << jsonEscape(sub.source)
        << "}";
    return oss.str();
}

std::string serializeProfileStats(const LeetCodeProfileStats& st) {
    std::ostringstream oss;
    oss << "{"
        << "\"username\":" << jsonEscape(st.username) << ","
        << "\"totalSolved\":" << st.totalSolved << ","
        << "\"easySolved\":" << st.easySolved << ","
        << "\"mediumSolved\":" << st.mediumSolved << ","
        << "\"hardSolved\":" << st.hardSolved << ","
        << "\"ranking\":" << st.ranking << ","
        << "\"lastProfileSync\":" << jsonEscape(st.lastProfileSync) << ","
        << "\"lastCatalogSync\":" << jsonEscape(st.lastCatalogSync) << ","
        << "\"lastSyncError\":" << jsonEscape(st.lastSyncError) << ","
        << "\"extensionLastSeen\":" << jsonEscape(st.extensionLastSeen) << ","
        << "\"extensionEventsCount\":" << st.extensionEventsCount
        << "}";
    return oss.str();
}

std::string serializeTopicItem(const TopicProgressItem& tp) {
    std::ostringstream oss;
    oss << std::fixed;
    oss.precision(1);
    oss << "{"
        << "\"topic\":" << jsonEscape(tp.topic) << ","
        << "\"totalProblems\":" << tp.totalProblems << ","
        << "\"solvedProblems\":" << tp.solvedProblems << ","
        << "\"attemptedProblems\":" << tp.attemptedProblems << ","
        << "\"remainingProblems\":" << tp.remainingProblems << ","
        << "\"easySolved\":" << tp.easySolved << ","
        << "\"mediumSolved\":" << tp.mediumSolved << ","
        << "\"hardSolved\":" << tp.hardSolved << ","
        << "\"easyTotal\":" << tp.easyTotal << ","
        << "\"mediumTotal\":" << tp.mediumTotal << ","
        << "\"hardTotal\":" << tp.hardTotal << ","
        << "\"goalTarget\":" << tp.goalTarget << ","
        << "\"completionPercent\":" << tp.completionPercent
        << "}";
    return oss.str();
}

} // namespace

HttpServer::HttpResponse HttpServer::handleLibSync() {
    std::size_t total = intel_.syncLeetCodeCatalogue();
    logger_.log("Library", "Sync", "Synced " + std::to_string(total) + " LeetCode problems");
    std::ostringstream oss;
    oss << "{\"ok\":true,\"totalCatalogue\":" << total
        << ",\"profile\":" << serializeProfileStats(intel_.profileStats()) << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleLibImport(const std::string& body) {
    std::size_t added = intel_.importCatalogueJson(body);
    logger_.log("Library", "Import", "Imported " + std::to_string(added) + " new problems");
    std::ostringstream oss;
    oss << "{\"ok\":true,\"added\":" << added
        << ",\"profile\":" << serializeProfileStats(intel_.profileStats()) << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleLibQuery(const std::string& body) const {
    JsonValue payload = JsonParser(body.empty() ? "{}" : body).parse();
    CatalogueQuery q;
    q.search = payload.getString("search");
    q.difficulty = payload.getString("difficulty", "all");
    q.topic = payload.getString("topic", "all");
    q.availability = payload.getString("availability", "all");
    q.status = payload.getString("status", "all");
    q.sortBy = payload.getString("sortBy", "id");
    q.sortDir = payload.getString("sortDir", "asc");
    q.page = static_cast<std::size_t>(std::max<long long>(1, payload.getInt("page", 1)));
    q.pageSize = static_cast<std::size_t>(std::max<long long>(10, payload.getInt("pageSize", 50)));

    const auto res = intel_.queryCatalogue(q);
    std::ostringstream oss;
    oss << "{\"ok\":true,"
        << "\"totalFiltered\":" << res.totalFiltered << ","
        << "\"totalCatalogue\":" << res.totalCatalogue << ","
        << "\"page\":" << res.page << ","
        << "\"pageSize\":" << res.pageSize << ","
        << "\"totalPages\":" << res.totalPages << ","
        << "\"availableTopics\":" << jsonStringArray(res.availableTopics) << ","
        << "\"profile\":" << serializeProfileStats(intel_.profileStats()) << ","
        << "\"items\":[";
    for (std::size_t i = 0; i < res.items.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& it = res.items[i];
        oss << "{"
            << "\"problem\":" << serializeCatalogProblem(it.problem) << ","
            << "\"progress\":" << serializeProblemProgress(it.progress) << ","
            << "\"hasNotes\":" << (it.hasNotes ? "true" : "false") << ","
            << "\"solutionCount\":" << it.solutionCount
            << "}";
    }
    oss << "]}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleLibDetail(const std::string& body) const {
    JsonValue payload = JsonParser(body).parse();
    const auto detail = intel_.getProblemDetail(payload.getString("slug"));
    std::ostringstream oss;
    oss << "{\"ok\":true,"
        << "\"problem\":" << serializeCatalogProblem(detail.problem) << ","
        << "\"progress\":" << serializeProblemProgress(detail.progress) << ","
        << "\"notes\":" << serializeNotes(detail.notes) << ","
        << "\"solutions\":[";
    for (std::size_t i = 0; i < detail.solutions.size(); ++i) {
        if (i > 0) oss << ",";
        oss << serializeSolution(detail.solutions[i]);
    }
    oss << "],\"submissions\":[";
    for (std::size_t i = 0; i < detail.submissions.size(); ++i) {
        if (i > 0) oss << ",";
        oss << serializeSubmission(detail.submissions[i]);
    }
    oss << "]}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleLibStatus(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string slug = payload.getString("slug");
    auto rec = intel_.updateProblemStatus(
        slug,
        payload.getString("status", ""),
        payload.has("bookmarked"),
        payload.getBool("bookmarked", false),
        payload.getBool("toggleBookmark", false),
        payload.has("needsRevision"),
        payload.getBool("needsRevision", false));
    logger_.log("Problem", slug, "Status -> " + rec.status);
    std::ostringstream oss;
    oss << "{\"ok\":true,\"progress\":" << serializeProblemProgress(rec) << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleLibNotes(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    ProblemNotes n;
    n.slug = payload.getString("slug");
    n.intuition = payload.getString("intuition");
    n.bruteForce = payload.getString("bruteForce");
    n.optimized = payload.getString("optimized");
    n.timeComplexity = payload.getString("timeComplexity");
    n.spaceComplexity = payload.getString("spaceComplexity");
    n.mistakesMade = payload.getString("mistakesMade");
    n.observations = payload.getString("observations");
    n.cppSnippet = payload.getString("cppSnippet");
    n.revisionComments = payload.getString("revisionComments");
    auto saved = intel_.saveProblemNotes(n);
    logger_.log("Notes", saved.slug, "Updated Markdown notes");
    std::ostringstream oss;
    oss << "{\"ok\":true,\"notes\":" << serializeNotes(saved) << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleNotesSearch(const std::string& body) const {
    JsonValue payload = JsonParser(body.empty() ? "{}" : body).parse();
    auto list = intel_.searchNotes(payload.getString("query"));
    std::ostringstream oss;
    oss << "{\"ok\":true,\"notes\":[";
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (i > 0) oss << ",";
        oss << serializeNotes(list[i]);
    }
    oss << "]}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleSolutionsAdd(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    SavedSolution s;
    s.slug = payload.getString("slug");
    s.problemTitle = payload.getString("problemTitle");
    s.approachName = payload.getString("approachName");
    s.timeComplexity = payload.getString("timeComplexity");
    s.spaceComplexity = payload.getString("spaceComplexity");
    s.explanation = payload.getString("explanation");
    s.cppCode = payload.getString("cppCode");
    s.topics = payload.getStringArray("topics");
    auto saved = intel_.addSolution(s);
    logger_.log("Solution", saved.problemTitle, saved.approachName);
    std::ostringstream oss;
    oss << "{\"ok\":true,\"solution\":" << serializeSolution(saved) << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleSolutionsDelete(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    intel_.deleteSolution(payload.getString("id"));
    return HttpResponse{200, "application/json; charset=utf-8", "{\"ok\":true}"};
}

HttpServer::HttpResponse HttpServer::handleSolutionsSearch(const std::string& body) const {
    JsonValue payload = JsonParser(body.empty() ? "{}" : body).parse();
    auto list = intel_.searchSolutions(
        payload.getString("query"),
        payload.getString("topic", "all"),
        payload.getString("complexity", "all"));
    std::ostringstream oss;
    oss << "{\"ok\":true,\"solutions\":[";
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (i > 0) oss << ",";
        oss << serializeSolution(list[i]);
    }
    oss << "]}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleTopicsGet() const {
    auto topics = intel_.computeTopicProgress();
    std::ostringstream oss;
    oss << "{\"ok\":true,\"topics\":[";
    for (std::size_t i = 0; i < topics.size(); ++i) {
        if (i > 0) oss << ",";
        oss << serializeTopicItem(topics[i]);
    }
    oss << "]}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleTopicsGoal(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string topic = payload.getString("topic");
    long long goal = payload.getInt("goalTarget", 10, true);
    intel_.setTopicGoal(topic, static_cast<std::size_t>(goal));
    return handleTopicsGet();
}

HttpServer::HttpResponse HttpServer::handleLcSync(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    std::string username = payload.getString("username");
    auto st = intel_.syncLeetCodeProfile(username);
    hub_.setPreferences(st.username, hub_.preferences().learningGoal);
    logger_.log("LeetCode", "Sync", "Synced public profile for " + st.username);
    return handleLcProfileGet();
}

HttpServer::HttpResponse HttpServer::handleLcImportSubmissions(const std::string& body) {
    std::size_t imported = intel_.importSubmissionsJson(body);
    logger_.log("LeetCode", "Import", "Imported " + std::to_string(imported) + " submissions");
    std::ostringstream oss;
    oss << "{\"ok\":true,\"imported\":" << imported
        << ",\"profile\":" << serializeProfileStats(intel_.profileStats()) << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleLcProfileGet() const {
    auto st = intel_.profileStats();
    auto subs = intel_.recentSubmissions(30);
    std::ostringstream oss;
    oss << "{\"ok\":true,\"profile\":" << serializeProfileStats(st) << ",\"submissions\":[";
    for (std::size_t i = 0; i < subs.size(); ++i) {
        if (i > 0) oss << ",";
        oss << serializeSubmission(subs[i]);
    }
    oss << "]}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleExtensionSubmission(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    auto sub = intel_.recordExtensionSubmission(
        static_cast<int>(payload.getInt("problemId", 0)),
        payload.getString("title"),
        payload.getString("slug"),
        payload.getString("difficulty"),
        payload.getString("timestamp"));
    logger_.log("Extension", "Accepted", sub.title);
    std::ostringstream oss;
    oss << "{\"ok\":true,\"submission\":" << serializeSubmission(sub) << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleExtensionStatus() const {
    auto st = intel_.profileStats();
    std::ostringstream oss;
    oss << "{\"ok\":true,"
        << "\"extensionLastSeen\":" << jsonEscape(st.extensionLastSeen) << ","
        << "\"extensionEventsCount\":" << st.extensionEventsCount << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleIntelRevisionGet() const {
    auto q = intel_.revisionQueue();
    std::ostringstream oss;
    oss << "{\"ok\":true,\"items\":[";
    for (std::size_t i = 0; i < q.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{"
            << "\"problem\":" << serializeCatalogProblem(q[i].problem) << ","
            << "\"progress\":" << serializeProblemProgress(q[i].progress) << ","
            << "\"hasNotes\":" << (q[i].hasNotes ? "true" : "false")
            << "}";
    }
    oss << "]}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleIntelRevisionReview(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    auto rec = intel_.reviewProblemRevision(payload.getString("slug"), payload.getString("rating"));
    logger_.log("Revision", rec.slug, "Reviewed (" + rec.lastRating + ")");
    return handleIntelRevisionGet();
}

HttpServer::HttpResponse HttpServer::handleIntelRevisionSchedule(const std::string& body) {
    JsonValue payload = JsonParser(body).parse();
    auto rec = intel_.scheduleProblemRevision(payload.getString("slug"), payload.getBool("needsRevision", true));
    std::ostringstream oss;
    oss << "{\"ok\":true,\"progress\":" << serializeProblemProgress(rec) << "}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

HttpServer::HttpResponse HttpServer::handleIntelAnalytics() const {
    const auto a = intel_.computeAnalytics();
    std::ostringstream oss;
    oss << std::fixed;
    oss.precision(1);
    oss << "{\"ok\":true,\"analytics\":{"
        << "\"totalCatalogue\":" << a.totalCatalogue << ","
        << "\"totalSolved\":" << a.totalSolved << ","
        << "\"totalAttempted\":" << a.totalAttempted << ","
        << "\"solvedEasy\":" << a.solvedEasy << ","
        << "\"solvedMedium\":" << a.solvedMedium << ","
        << "\"solvedHard\":" << a.solvedHard << ","
        << "\"overallTopicCompletionPercent\":" << a.overallTopicCompletionPercent << ","
        << "\"currentStreak\":" << a.currentStreak << ","
        << "\"revisionCompleted\":" << a.revisionCompleted << ","
        << "\"revisionDue\":" << a.revisionDue << ","
        << "\"needsRevisionCount\":" << a.needsRevisionCount << ","
        << "\"totalSolutions\":" << a.totalSolutions << ","
        << "\"totalNotes\":" << a.totalNotes << ","
        << "\"weeklyActivity\":[";
    for (std::size_t i = 0; i < a.weeklyActivity.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{\"date\":" << jsonEscape(a.weeklyActivity[i].date)
            << ",\"count\":" << a.weeklyActivity[i].count << "}";
    }
    oss << "],\"dailyActivityHeatmap\":[";
    for (std::size_t i = 0; i < a.dailyActivityHeatmap.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{\"date\":" << jsonEscape(a.dailyActivityHeatmap[i].date)
            << ",\"count\":" << a.dailyActivityHeatmap[i].count << "}";
    }
    oss << "],\"weakestTopics\":[";
    for (std::size_t i = 0; i < a.weakestTopics.size(); ++i) {
        if (i > 0) oss << ",";
        oss << serializeTopicItem(a.weakestTopics[i]);
    }
    oss << "],\"mostPracticedTopics\":[";
    for (std::size_t i = 0; i < a.mostPracticedTopics.size(); ++i) {
        if (i > 0) oss << ",";
        oss << serializeTopicItem(a.mostPracticedTopics[i]);
    }
    oss << "]}}";
    return HttpResponse{200, "application/json; charset=utf-8", oss.str()};
}

} // namespace dsa::server
