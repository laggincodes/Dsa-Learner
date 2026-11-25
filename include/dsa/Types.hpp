#pragma once

#include <cstddef>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace dsa {

// ============================================================================
// Exception Hierarchy
// ============================================================================

class DSAException : public std::runtime_error {
public:
    explicit DSAException(const std::string& message, std::string code = "DSA_ERROR")
        : std::runtime_error(message), code_(std::move(code)) {}

    const std::string& code() const noexcept { return code_; }

private:
    std::string code_;
};

class ValidationException : public DSAException {
public:
    explicit ValidationException(const std::string& message)
        : DSAException(message, "VALIDATION_ERROR") {}
};

class OverflowException : public DSAException {
public:
    explicit OverflowException(const std::string& message)
        : DSAException(message, "OVERFLOW") {}
};

class UnderflowException : public DSAException {
public:
    explicit UnderflowException(const std::string& message)
        : DSAException(message, "UNDERFLOW") {}
};

class FileIOException : public DSAException {
public:
    explicit FileIOException(const std::string& message)
        : DSAException(message, "FILE_IO_ERROR") {}
};

// ============================================================================
// Core Value Types & Operator Overloading
// ============================================================================

struct Metrics {
    std::size_t comparisons = 0;
    std::size_t swaps = 0;

    Metrics operator+(const Metrics& other) const {
        return Metrics{comparisons + other.comparisons, swaps + other.swaps};
    }

    Metrics& operator+=(const Metrics& other) {
        comparisons += other.comparisons;
        swaps += other.swaps;
        return *this;
    }

    bool operator==(const Metrics& other) const {
        return comparisons == other.comparisons && swaps == other.swaps;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Metrics& m) {
    os << "Metrics(comparisons=" << m.comparisons << ", swaps=" << m.swaps << ")";
    return os;
}

struct AlgorithmInfo {
    std::string id;
    std::string name;
    std::string category;
    std::string description;
    std::string bestTime;
    std::string averageTime;
    std::string worstTime;
    std::string spaceComplexity;
    std::string notes;

    bool operator==(const AlgorithmInfo& other) const {
        return id == other.id && category == other.category;
    }
};

struct SortStep {
    std::vector<int> array;
    int indexA = -1;
    int indexB = -1;
    std::vector<int> sortedIndices;
    std::string action; // "init", "compare", "swap", "shift", "insert", "done"
    std::string detail;
    std::size_t comparisons = 0;
    std::size_t swaps = 0;
};

struct SortResult {
    AlgorithmInfo info;
    std::vector<int> initialArray;
    std::vector<int> sortedArray;
    std::vector<SortStep> steps;
    Metrics metrics;
};

struct SearchStep {
    int index = -1;
    int low = -1;
    int high = -1;
    int mid = -1;
    int value = 0;
    std::string status; // "inspect", "found", "discard_left", "discard_right", "not_found"
    std::string detail;
    std::size_t comparisons = 0;
};

struct SearchResult {
    AlgorithmInfo info;
    std::vector<int> arrayUsed;
    int target = 0;
    bool found = false;
    int foundIndex = -1;
    std::size_t comparisons = 0;
    std::vector<SearchStep> steps;
};

struct DataStructureState {
    std::string name;
    std::vector<int> elements;
    std::size_t size = 0;
    std::size_t capacity = 0;
    bool isEmpty = true;
    bool isFull = false;
    bool hasTopOrFront = false;
    int topOrFrontValue = 0;
};

struct OperationRecord {
    std::string timestamp;
    std::string category;
    std::string operation;
    std::string summary;
};

// ============================================================================
// Upgraded Module Types (Comparator, BST, Graph, Quiz, Progress, Code)
// ============================================================================

struct BenchmarkResult {
    AlgorithmInfo info;
    std::size_t inputSize = 0;
    std::size_t comparisons = 0;
    std::size_t swaps = 0;
    double elapsedMicroseconds = 0.0;
    long long elapsedNanoseconds = 0;
};

struct TreeNodeSnapshot {
    int value = 0;
    int depth = 0;
    int orderIndex = 0;
    bool hasLeft = false;
    int leftValue = 0;
    bool hasRight = false;
    int rightValue = 0;
};

struct TreeStep {
    int value = 0;
    std::string action; // "visit", "go_left", "go_right", "insert", "found", "delete", "not_found", "traverse"
    std::string detail;
};

struct BstOperationResult {
    bool success = true;
    std::string operation;
    std::string message;
    std::vector<int> traversalOrder;
    std::vector<TreeStep> steps;
    std::vector<TreeNodeSnapshot> nodes;
    std::size_t nodeCount = 0;
    int height = 0;
};

struct GraphEdge {
    std::string from;
    std::string to;
};

struct GraphStep {
    std::string current;
    std::string edgeFrom;
    std::string edgeTo;
    std::vector<std::string> visited;
    std::vector<std::string> frontier;
    std::string action; // "start", "visit", "discover", "done"
    std::string detail;
};

struct GraphTraversalResult {
    std::string algorithm; // "bfs" or "dfs"
    std::string startVertex;
    std::vector<std::string> order;
    std::vector<GraphStep> steps;
};

struct GraphSnapshot {
    bool directed = false;
    std::vector<std::string> vertices;
    std::vector<GraphEdge> edges;
};

struct QuizQuestion {
    int id = 0;
    std::string category;
    std::string question;
    std::vector<std::string> options;
    int correctIndex = 0;
    std::string explanation;
};

struct QuizAnswerResult {
    int questionId = 0;
    bool correct = false;
    int selectedIndex = -1;
    int correctIndex = 0;
    std::string explanation;
    std::size_t currentScore = 0;
    std::size_t answeredCount = 0;
    std::size_t totalQuestions = 0;
};

struct QuizRecord {
    std::string timestamp;
    std::string category;
    std::size_t score = 0;
    std::size_t total = 0;
    double accuracy = 0.0;
};

struct ModuleProgress {
    std::string id;
    std::string name;
    bool completed = false;
};

struct ProgressSnapshot {
    std::vector<ModuleProgress> modules;
    std::size_t completedCount = 0;
    std::size_t totalModules = 0;
    double completionPercent = 0.0;
    std::size_t totalQuizzesAttempted = 0;
    double averageAccuracy = 0.0;
    double bestAccuracy = 0.0;
    std::vector<QuizRecord> quizHistory;
};

struct AlgorithmCodeEntry {
    std::string id;
    std::string name;
    std::string category;
    std::string bestTime;
    std::string averageTime;
    std::string worstTime;
    std::string spaceComplexity;
    std::string cppCode;
    std::string pseudocode;
};

} // namespace dsa
