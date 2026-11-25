#include "dsa/QuizEngine.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>

namespace dsa {

QuizEngine::QuizEngine() {
    bank_ = {
        {1, "Sorting",
         "What is the best-case time complexity of Bubble Sort when using an early-exit swapped flag?",
         {"O(n)", "O(n log n)", "O(n^2)", "O(1)"},
         0,
         "When the array is already sorted, the first pass performs zero swaps and exits in O(n) comparisons."},
        {2, "Sorting",
         "Which sorting algorithm guarantees at most (n - 1) swaps in the worst case?",
         {"Bubble Sort", "Selection Sort", "Insertion Sort", "Quick Sort"},
         1,
         "Selection Sort finds the minimum element per pass and performs at most one swap per outer iteration."},
        {3, "Sorting",
         "Why is Insertion Sort often faster than Bubble Sort on nearly sorted arrays?",
         {"It uses O(n) extra space", "It only shifts elements that are out of order", "It divides the array in half", "It runs in O(log n) worst case"},
         1,
         "Insertion Sort's inner loop stops immediately when arr[j - 1] <= key, achieving O(n) on nearly sorted inputs."},
        {4, "Searching",
         "What precondition is strictly required for Binary Search to work correctly?",
         {"Array length must be even", "Array must have unique values", "Array must be sorted", "Array must fit in a stack"},
         2,
         "Binary Search discards half the search space based on ordering, which requires the input to be sorted."},
        {5, "Searching",
         "In the worst case, approximately how many comparisons does Binary Search need on 1,024 elements?",
         {"512", "100", "10 to 11", "1"},
         2,
         "Because log2(1024) = 10, Binary Search inspects at most ~10-11 elements."},
        {6, "Searching",
         "When is Linear Search preferable over Binary Search?",
         {"When the dataset is small or unsorted", "When elements are guaranteed sorted", "When O(log n) is required", "Only on linked trees"},
         0,
         "Sorting an unsorted array costs O(n log n), so a single lookup on unsorted data is faster with O(n) Linear Search."},
        {7, "Stacks & Queues",
         "Which principle describes how a Stack operates?",
         {"First-In, First-Out (FIFO)", "Last-In, First-Out (LIFO)", "Smallest-In, First-Out", "Random Access"},
         1,
         "A Stack pushes and pops from the same end (the top), making it Last-In, First-Out (LIFO)."},
        {8, "Stacks & Queues",
         "What exception condition occurs when calling Pop() or Front() on an empty container?",
         {"Overflow", "Underflow", "Segmentation", "Deadlock"},
         1,
         "Attempting to remove or read from an empty Stack or Queue causes an Underflow."},
        {9, "Stacks & Queues",
         "Which data structure is used internally to manage vertices in Breadth-First Search (BFS)?",
         {"Stack", "Queue", "Binary Search Tree", "Hash Set only"},
         1,
         "BFS explores vertices level by level using a FIFO Queue."},
        {10, "Trees",
         "Which traversal of a Binary Search Tree (BST) always produces values in ascending sorted order?",
         {"Preorder", "Inorder", "Postorder", "Level-order"},
         1,
         "Inorder traversal visits Left -> Root -> Right, yielding sorted values in a valid BST."},
        {11, "Trees",
         "When deleting a BST node that has two children, what commonly replaces its value?",
         {"The root node", "The inorder successor (smallest in right subtree)", "Any leaf node", "Its parent node"},
         1,
         "Replacing with the inorder successor (or predecessor) preserves the BST ordering invariant."},
        {12, "Trees",
         "What is the worst-case search time complexity in an unbalanced Binary Search Tree of n nodes?",
         {"O(1)", "O(log n)", "O(n)", "O(n log n)"},
         2,
         "If keys are inserted in sorted order, the BST degenerates into a skewed chain of height n, giving O(n) search."},
        {13, "Time Complexity",
         "What is the auxiliary space complexity of in-place Bubble, Selection, and Insertion Sort?",
         {"O(1)", "O(log n)", "O(n)", "O(n^2)"},
         0,
         "All three sort elements in place using only a constant number of temporary variables."},
        {14, "Time Complexity",
         "What is the time complexity of BFS and DFS on a graph with V vertices and E edges using adjacency lists?",
         {"O(V * E)", "O(V + E)", "O(V^2 log V)", "O(log V)"},
         1,
         "Each vertex and each edge is processed a constant number of times, yielding O(V + E)."},
        {15, "Time Complexity",
         "Which growth rate grows the slowest as input size n becomes very large?",
         {"O(n)", "O(n log n)", "O(log n)", "O(n^2)"},
         2,
         "Logarithmic time O(log n) grows much slower than linear, linearithmic, or quadratic complexities."}
    };
}

const QuizQuestion& QuizEngine::findQuestion(int id) const {
    for (const auto& q : bank_) {
        if (q.id == id) return q;
    }
    throw ValidationException("Quiz question ID not found.");
}

QuizEngine::StartSessionResult QuizEngine::startSession(std::size_t count, const std::string& category) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<QuizQuestion> pool;
    for (const auto& q : bank_) {
        if (category == "all" || category.empty() || q.category == category) {
            pool.push_back(q);
        }
    }
    if (pool.empty()) {
        throw ValidationException("No quiz questions found for category '" + category + "'.");
    }

    static thread_local std::mt19937 rng(std::random_device{}());
    std::shuffle(pool.begin(), pool.end(), rng);

    if (count < 1) count = 1;
    if (count > pool.size()) count = pool.size();
    pool.resize(count);

    const std::string sid = "quiz-" + std::to_string(nextSessionSeq_++);
    QuizSession session;
    session.category = (category.empty() ? "all" : category);
    for (const auto& q : pool) {
        session.questionIds.push_back(q.id);
    }
    sessions_[sid] = std::move(session);

    return StartSessionResult{sid, std::move(pool)};
}

QuizAnswerResult QuizEngine::submitAnswer(const std::string& sessionId, int questionId, int selectedIndex) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        throw ValidationException("Invalid or expired quiz session.");
    }
    QuizSession& session = it->second;
    if (session.finished) {
        throw ValidationException("This quiz session has already been finished.");
    }
    if (std::find(session.questionIds.begin(), session.questionIds.end(), questionId) == session.questionIds.end()) {
        throw ValidationException("Question does not belong to this quiz session.");
    }
    if (session.answers.find(questionId) != session.answers.end()) {
        throw ValidationException("Question has already been answered.");
    }

    const QuizQuestion& q = findQuestion(questionId);
    if (selectedIndex < 0 || static_cast<std::size_t>(selectedIndex) >= q.options.size()) {
        throw ValidationException("Selected answer option index is out of range.");
    }

    const bool correct = (selectedIndex == q.correctIndex);
    session.answers[questionId] = selectedIndex;
    if (correct) {
        ++session.score;
    }

    return QuizAnswerResult{
        questionId,
        correct,
        selectedIndex,
        q.correctIndex,
        q.explanation,
        session.score,
        session.answers.size(),
        session.questionIds.size()};
}

QuizRecord QuizEngine::finishSession(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        throw ValidationException("Invalid or expired quiz session.");
    }
    QuizSession& session = it->second;
    session.finished = true;

    const std::size_t total = session.questionIds.size();
    const double accuracy = total > 0 ? (100.0 * static_cast<double>(session.score) / static_cast<double>(total)) : 0.0;

    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
    localtime_r(&t, &tmBuf);
    std::ostringstream ts;
    ts << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S");

    return QuizRecord{ts.str(), session.category, session.score, total, accuracy};
}

} // namespace dsa
