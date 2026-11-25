#include "dsa/ProgressManager.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace dsa {

ProgressManager::ProgressManager(std::string filePath) : filePath_(std::move(filePath)) {
    initDefaultModules();
    loadFromDisk();
}

void ProgressManager::initDefaultModules() {
    modules_ = {
        {"sorting", "Sorting Visualizer", false},
        {"searching", "Searching Visualizer", false},
        {"datastructures", "Stack & Queue Playground", false},
        {"comparator", "Complexity Comparator", false},
        {"bst", "Binary Search Tree", false},
        {"graph", "Graph Visualizer (BFS/DFS)", false},
        {"quiz", "Quiz Arena", false},
        {"codeviewer", "Algorithm Code Viewer", false},
    };
}

std::string ProgressManager::sanitize(const std::string& s) {
    std::string out = s;
    std::replace(out.begin(), out.end(), '|', '/');
    std::replace(out.begin(), out.end(), '\n', ' ');
    std::replace(out.begin(), out.end(), '\r', ' ');
    return out;
}

void ProgressManager::loadFromDisk() {
    std::ifstream in(filePath_);
    if (!in.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string item;
        while (std::getline(ss, item, '|')) {
            parts.push_back(item);
        }
        if (parts.size() >= 3 && parts[0] == "MODULE") {
            for (auto& m : modules_) {
                if (m.id == parts[1]) {
                    m.completed = (parts[2] == "1");
                    break;
                }
            }
        } else if (parts.size() >= 6 && parts[0] == "QUIZ") {
            try {
                QuizRecord rec;
                rec.timestamp = parts[1];
                rec.category = parts[2];
                rec.score = static_cast<std::size_t>(std::stoul(parts[3]));
                rec.total = static_cast<std::size_t>(std::stoul(parts[4]));
                rec.accuracy = std::stod(parts[5]);
                quizHistory_.push_back(std::move(rec));
            } catch (...) {
                // Skip malformed record line cleanly
            }
        }
    }
}

void ProgressManager::saveToDisk() const {
    std::ofstream out(filePath_, std::ios::trunc);
    if (!out.is_open()) {
        throw FileIOException("Failed to write progress file: " + filePath_);
    }

    for (const auto& m : modules_) {
        out << "MODULE|" << sanitize(m.id) << "|" << (m.completed ? "1" : "0") << "\n";
    }
    for (const auto& q : quizHistory_) {
        out << "QUIZ|" << sanitize(q.timestamp) << "|" << sanitize(q.category) << "|"
            << q.score << "|" << q.total << "|" << q.accuracy << "\n";
    }
    out.flush();
}

void ProgressManager::markModule(const std::string& moduleId, bool completed) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool found = false;
    for (auto& m : modules_) {
        if (m.id == moduleId) {
            if (m.completed == completed) {
                return;
            }
            m.completed = completed;
            found = true;
            break;
        }
    }
    if (!found) {
        throw ValidationException("Unknown module ID: '" + moduleId + "'.");
    }
    saveToDisk();
}

void ProgressManager::recordQuiz(const QuizRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    quizHistory_.push_back(record);
    for (auto& m : modules_) {
        if (m.id == "quiz") {
            m.completed = true;
            break;
        }
    }
    saveToDisk();
}

ProgressSnapshot ProgressManager::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    ProgressSnapshot snap;
    snap.modules = modules_;
    snap.totalModules = modules_.size();
    for (const auto& m : modules_) {
        if (m.completed) ++snap.completedCount;
    }
    snap.completionPercent = snap.totalModules > 0
        ? (100.0 * static_cast<double>(snap.completedCount) / static_cast<double>(snap.totalModules))
        : 0.0;

    snap.totalQuizzesAttempted = quizHistory_.size();
    double sumAcc = 0.0;
    double bestAcc = 0.0;
    for (const auto& q : quizHistory_) {
        sumAcc += q.accuracy;
        if (q.accuracy > bestAcc) bestAcc = q.accuracy;
    }
    snap.averageAccuracy = quizHistory_.empty() ? 0.0 : (sumAcc / static_cast<double>(quizHistory_.size()));
    snap.bestAccuracy = bestAcc;

    snap.quizHistory = quizHistory_;
    std::reverse(snap.quizHistory.begin(), snap.quizHistory.end());
    if (snap.quizHistory.size() > 20) {
        snap.quizHistory.resize(20);
    }
    return snap;
}

void ProgressManager::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    initDefaultModules();
    quizHistory_.clear();
    saveToDisk();
}

} // namespace dsa
