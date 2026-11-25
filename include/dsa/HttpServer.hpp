#pragma once

#include "dsa/BinarySearchTree.hpp"
#include "dsa/DataStructures.hpp"
#include "dsa/Graph.hpp"
#include "dsa/HistoryLogger.hpp"
#include "dsa/IntelligenceHub.hpp"
#include "dsa/LearningHub.hpp"
#include "dsa/ProgressManager.hpp"
#include "dsa/QuizEngine.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

namespace dsa::server {

class HttpServer {
public:
    HttpServer(std::uint16_t port, std::string webRoot, HistoryLogger& logger,
               ProgressManager& progress, LearningHub& hub, IntelligenceHub& intel);
    ~HttpServer();

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    void start();
    void stop();
    std::uint16_t port() const noexcept { return port_; }

private:
    struct HttpRequest {
        std::string method;
        std::string path;
        std::string body;
    };

    struct HttpResponse {
        int statusCode = 200;
        std::string contentType = "application/json; charset=utf-8";
        std::string body;
    };

    void handleClient(int clientFd);
    HttpResponse routeRequest(const HttpRequest& req);
    HttpResponse serveStaticFile(const std::string& requestPath) const;

    HttpResponse handleOverview() const;
    HttpResponse handleGenerateArray(const std::string& body) const;
    HttpResponse handleSort(const std::string& body);
    HttpResponse handleSearch(const std::string& body);
    HttpResponse handleDsState();
    HttpResponse handleStackOp(const std::string& body);
    HttpResponse handleQueueOp(const std::string& body);
    HttpResponse handleListOp(const std::string& body);
    HttpResponse handleGetHistory() const;
    HttpResponse handleClearHistory();

    // New module handlers
    HttpResponse handleCompare(const std::string& body);
    HttpResponse handleBstState();
    HttpResponse handleBstOp(const std::string& body);
    HttpResponse handleGraphState();
    HttpResponse handleGraphOp(const std::string& body);
    HttpResponse handleQuizStart(const std::string& body);
    HttpResponse handleQuizAnswer(const std::string& body);
    HttpResponse handleQuizFinish(const std::string& body);
    HttpResponse handleGetProgress() const;
    HttpResponse handleMarkModule(const std::string& body);
    HttpResponse handleResetProgress();
    HttpResponse handleGetCode();

    // Ecosystem handlers
    HttpResponse handleEcosystemState() const;
    HttpResponse handleSavePreferences(const std::string& body);
    HttpResponse handleToggleRoadmap(const std::string& body);
    HttpResponse handlePracticeUpdate(const std::string& body);
    HttpResponse handleLeetCodeAdd(const std::string& body);
    HttpResponse handleLeetCodeUpdate(const std::string& body);
    HttpResponse handleLeetCodeDelete(const std::string& body);
    HttpResponse handleRevisionReview(const std::string& body);
    HttpResponse handleRevisionMistake(const std::string& body);
    HttpResponse handleAnalytics() const;

    // Intelligence Hub handlers
    HttpResponse handleLibSync();
    HttpResponse handleLibImport(const std::string& body);
    HttpResponse handleLibQuery(const std::string& body) const;
    HttpResponse handleLibDetail(const std::string& body) const;
    HttpResponse handleLibStatus(const std::string& body);
    HttpResponse handleLibNotes(const std::string& body);
    HttpResponse handleNotesSearch(const std::string& body) const;
    HttpResponse handleSolutionsAdd(const std::string& body);
    HttpResponse handleSolutionsDelete(const std::string& body);
    HttpResponse handleSolutionsSearch(const std::string& body) const;
    HttpResponse handleTopicsGet() const;
    HttpResponse handleTopicsGoal(const std::string& body);
    HttpResponse handleLcSync(const std::string& body);
    HttpResponse handleLcImportSubmissions(const std::string& body);
    HttpResponse handleLcProfileGet() const;
    HttpResponse handleExtensionSubmission(const std::string& body);
    HttpResponse handleExtensionStatus() const;
    HttpResponse handleIntelRevisionGet() const;
    HttpResponse handleIntelRevisionReview(const std::string& body);
    HttpResponse handleIntelRevisionSchedule(const std::string& body);
    HttpResponse handleIntelAnalytics() const;

    std::uint16_t port_;
    std::string webRoot_;
    HistoryLogger& logger_;
    ProgressManager& progress_;
    LearningHub& hub_;
    IntelligenceHub& intel_;

    BoundedStack stack_;
    BoundedQueue queue_;
    BoundedLinkedList linkedList_;
    BinarySearchTree bst_;
    Graph graph_;
    QuizEngine quiz_;
    std::mutex dsMutex_;

    int serverFd_ = -1;
    std::atomic<bool> running_{false};
};

} // namespace dsa::server
