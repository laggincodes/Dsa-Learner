#include "dsa/HistoryLogger.hpp"
#include "dsa/HttpServer.hpp"
#include "dsa/IntelligenceHub.hpp"
#include "dsa/LearningHub.hpp"
#include "dsa/ProgressManager.hpp"

#include <csignal>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

#ifndef DSA_DEFAULT_WEB_DIR
#define DSA_DEFAULT_WEB_DIR "web"
#endif

namespace {

dsa::server::HttpServer* g_server = nullptr;

void handleSignal(int) {
    if (g_server) {
        g_server->stop();
    }
}

std::string resolveWebRoot(const std::string& cliWebDir) {
    namespace fs = std::filesystem;
    if (!cliWebDir.empty() && fs::exists(cliWebDir)) {
        return cliWebDir;
    }
    if (fs::exists("web/index.html")) {
        return "web";
    }
    if (fs::exists("../web/index.html")) {
        return "../web";
    }
    if (fs::exists(fs::path(DSA_DEFAULT_WEB_DIR) / "index.html")) {
        return DSA_DEFAULT_WEB_DIR;
    }
    return DSA_DEFAULT_WEB_DIR;
}

} // namespace

int main(int argc, char* argv[]) {
    std::uint16_t port = 8080;
    std::string cliWebDir;
    std::string logPath = "dsa_history.log";
    std::string progressPath = "dsa_progress.dat";
    std::string ecosystemPath = "dsa_ecosystem.dat";
    std::string catalogPath = "dsa_catalogue.dat";
    std::string intelPath = "dsa_intelligence.dat";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            port = static_cast<std::uint16_t>(std::stoi(argv[++i]));
        } else if ((arg == "--web" || arg == "-w") && i + 1 < argc) {
            cliWebDir = argv[++i];
        } else if ((arg == "--log" || arg == "-l") && i + 1 < argc) {
            logPath = argv[++i];
        } else if ((arg == "--progress") && i + 1 < argc) {
            progressPath = argv[++i];
        } else if ((arg == "--ecosystem") && i + 1 < argc) {
            ecosystemPath = argv[++i];
        } else if ((arg == "--catalogue") && i + 1 < argc) {
            catalogPath = argv[++i];
        } else if ((arg == "--intel") && i + 1 < argc) {
            intelPath = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: dsa_server [--port 8080] [--web path/to/web] [--log dsa_history.log] [--progress dsa_progress.dat] [--ecosystem dsa_ecosystem.dat] [--catalogue dsa_catalogue.dat] [--intel dsa_intelligence.dat]\n";
            return 0;
        }
    }

    try {
        dsa::HistoryLogger logger(logPath);
        dsa::ProgressManager progress(progressPath);
        dsa::LearningHub hub(ecosystemPath);
        dsa::IntelligenceHub intel(catalogPath, intelPath);
        const std::string webRoot = resolveWebRoot(cliWebDir);
        dsa::server::HttpServer server(port, webRoot, logger, progress, hub, intel);
        g_server = &server;
        std::signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);
        server.start();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
