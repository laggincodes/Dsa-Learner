#include "dsa/Algorithms.hpp"
#include "dsa/BinarySearchTree.hpp"
#include "dsa/CodeRegistry.hpp"
#include "dsa/DataStructures.hpp"
#include "dsa/Graph.hpp"
#include "dsa/HistoryLogger.hpp"
#include "dsa/IntelligenceHub.hpp"
#include "dsa/LearningHub.hpp"
#include "dsa/ProgressManager.hpp"
#include "dsa/QuizEngine.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <vector>

using namespace dsa;

namespace {

void testTemplatesAndOverloads() {
    int a = 10, b = 20;
    utils::swapValues(a, b);
    assert(a == 20 && b == 10);

    std::vector<int> sorted{1, 2, 3, 5};
    std::vector<int> unsorted{3, 1, 2};
    assert(utils::isSorted(sorted));
    assert(!utils::isSorted(unsorted));
    assert(utils::joinValues(sorted) == "1, 2, 3, 5");

    auto rand1 = utils::generateRandomArray(10);
    auto rand2 = utils::generateRandomArray(10, -50, 50);
    assert(rand1.size() == 10);
    assert(rand2.size() == 10);
    for (int v : rand2) {
        assert(v >= -50 && v <= 50);
    }

    Metrics m1{3, 2};
    Metrics m2{4, 1};
    Metrics m3 = m1 + m2;
    assert((m3 == Metrics{7, 3}));
    std::ostringstream oss;
    oss << m3;
    assert(oss.str().find("comparisons=7") != std::string::npos);
}

void testSortingAlgorithms() {
    const std::vector<std::vector<int>> cases = {
        {64, 34, 25, 12, 22, 11, 90},
        {5, 4, 3, 2, 1},
        {1, 2, 3, 4, 5},
        {42, 42, 7, -3, 0, 7},
        {9, 1}
    };

    const std::vector<std::string> algos = {"bubble", "selection", "insertion"};
    for (const auto& algoId : algos) {
        auto sorter = createSorter(algoId);
        for (const auto& input : cases) {
            SortResult res = sorter->sort(input);
            assert(utils::isSorted(res.sortedArray));
            assert(res.sortedArray.size() == input.size());
            assert(!res.steps.empty());
            assert(res.steps.back().action == "done");
            assert(res.steps.back().array == res.sortedArray);
        }
    }

    bool threw = false;
    try {
        createSorter("bubble")->sort({1});
    } catch (const ValidationException&) {
        threw = true;
    }
    assert(threw);
}

void testSearchingAlgorithms() {
    std::vector<int> unsorted{15, 3, 42, 8, 27};
    auto linear = createSearcher("linear");
    auto res1 = linear->search(unsorted, 42);
    assert(res1.found && res1.foundIndex == 2 && res1.comparisons == 3);

    auto res2 = linear->search(unsorted, 99);
    assert(!res2.found && res2.foundIndex == -1 && res2.comparisons == 5);

    auto binary = createSearcher("binary");
    bool threwUnsorted = false;
    try {
        binary->search(unsorted, 42);
    } catch (const ValidationException&) {
        threwUnsorted = true;
    }
    assert(threwUnsorted);

    std::vector<int> sorted{3, 8, 15, 27, 42, 56};
    auto b1 = binary->search(sorted, 27);
    assert(b1.found && b1.foundIndex == 3);

    auto b2 = binary->search(sorted, 100);
    assert(!b2.found && b2.foundIndex == -1);
}

void testStackAndQueue() {
    BoundedStack stack(3);
    assert(stack.isEmpty());
    stack.push(10);
    stack += 20;
    stack.push(30);
    assert(stack.isFull());
    assert(stack.peek() == 30);

    bool overflowed = false;
    try {
        stack.push(40);
    } catch (const OverflowException&) {
        overflowed = true;
    }
    assert(overflowed);

    assert(stack.pop() == 30);
    assert(stack.pop() == 20);
    assert(stack.pop() == 10);
    assert(stack.isEmpty());

    bool underflowed = false;
    try {
        stack.pop();
    } catch (const UnderflowException&) {
        underflowed = true;
    }
    assert(underflowed);

    stack.push(std::vector<int>{1, 2});
    assert(stack.size() == 2);
    stack.clear();
    assert(stack.isEmpty());

    BoundedQueue queue(3);
    assert(queue.isEmpty());
    queue.enqueue(100);
    queue += 200;
    queue.enqueue(300);
    assert(queue.isFull());
    assert(queue.front() == 100);

    bool qOverflow = false;
    try {
        queue.enqueue(400);
    } catch (const OverflowException&) {
        qOverflow = true;
    }
    assert(qOverflow);

    assert(queue.dequeue() == 100);
    assert(queue.dequeue() == 200);
    assert(queue.dequeue() == 300);

    bool qUnderflow = false;
    try {
        queue.front();
    } catch (const UnderflowException&) {
        qUnderflow = true;
    }
    assert(qUnderflow);

    BoundedLinkedList list(4);
    list.insertTail(10);
    list.insertHead(5);
    list.insertTail(20);
    assert(list.size() == 3);
    assert(list.findIndex(10) == 1);
    assert(list.removeValue(10));
    assert(list.removeHead() == 5);
    assert(list.size() == 1);
}

void testHistoryLogger() {
    const std::string testLog = "/tmp/dsa_manager_lite_test.log";
    std::filesystem::remove(testLog);

    {
        HistoryLogger logger(testLog);
        logger.log("Sorting", "Bubble Sort", "n=7, comparisons=15, swaps=8");
        logger.log("Stack", "Push", "Pushed 42");
        auto records = logger.readRecent(10);
        assert(records.size() == 2);
        assert(records[0].operation == "Push");
        assert(records[1].operation == "Bubble Sort");
        logger.clear();
        assert(logger.readRecent(10).empty());
    }

    std::filesystem::remove(testLog);
}

void testComparator() {
    auto arr = utils::generatePatternArray(100, "reversed");
    assert(arr.size() == 100 && arr[0] == 100 && arr[99] == 1);
    auto results = compareSortingAlgorithms(arr);
    assert(results.size() == 3);
    for (const auto& r : results) {
        assert(r.inputSize == 100);
        assert(r.comparisons > 0);
        assert(r.elapsedNanoseconds >= 0);
    }
}

void testBinarySearchTree() {
    BinarySearchTree bst;
    bool emptyErr = false;
    try {
        bst.search(10);
    } catch (const UnderflowException&) {
        emptyErr = true;
    }
    assert(emptyErr);

    bst.loadSample();
    assert(bst.size() == 7);
    assert(bst.traverse("inorder").traversalOrder == (std::vector<int>{20, 30, 40, 50, 60, 70, 80}));
    assert(bst.traverse("preorder").traversalOrder == (std::vector<int>{50, 30, 20, 40, 70, 60, 80}));
    assert(bst.traverse("postorder").traversalOrder == (std::vector<int>{20, 40, 30, 60, 80, 70, 50}));
    assert(bst.search(40).success);
    assert(!bst.search(99).success);

    // Delete 2-children root node (50) -> replaced with inorder successor (60)
    bst.remove(50);
    assert(bst.size() == 6);
    assert(bst.traverse("inorder").traversalOrder == (std::vector<int>{20, 30, 40, 60, 70, 80}));
}

void testGraph() {
    Graph g;
    bool emptyErr = false;
    try {
        g.bfs("A");
    } catch (const UnderflowException&) {
        emptyErr = true;
    }
    assert(emptyErr);

    g.loadSample();
    auto bfsRes = g.bfs("A");
    assert(bfsRes.order.size() == 7 && bfsRes.order[0] == "A");
    auto dfsRes = g.dfs("A");
    assert(dfsRes.order.size() == 7 && dfsRes.order[0] == "A");
}

void testQuizAndProgress() {
    const std::string progPath = "/tmp/dsa_progress_test.dat";
    std::filesystem::remove(progPath);

    QuizEngine quiz;
    auto started = quiz.startSession(5, "all");
    assert(started.questions.size() == 5);
    for (const auto& q : started.questions) {
        auto ans = quiz.submitAnswer(started.sessionId, q.id, 0);
        assert(ans.answeredCount >= 1);
    }
    auto rec = quiz.finishSession(started.sessionId);
    assert(rec.total == 5);

    {
        ProgressManager pm(progPath);
        pm.markModule("bst", true);
        pm.recordQuiz(rec);
        auto snap = pm.snapshot();
        assert(snap.totalQuizzesAttempted == 1);
        assert(snap.completedCount >= 2);
    }

    // Verify persistence across new ProgressManager instance
    {
        ProgressManager pmReloaded(progPath);
        auto snap = pmReloaded.snapshot();
        assert(snap.totalQuizzesAttempted == 1);
        assert(snap.completedCount >= 2);
    }

    assert(getAllAlgorithmCodeEntries().size() >= 8);
    std::filesystem::remove(progPath);
}

void testLearningHubPersistence() {
    const std::string ecoPath = "/tmp/dsa_eco_test.dat";
    const std::string progPath = "/tmp/dsa_eco_prog_test.dat";
    const std::string logPath = "/tmp/dsa_eco_log_test.log";
    for (const auto& p : {ecoPath, progPath, logPath}) {
        std::filesystem::remove(p);
    }

    {
        LearningHub hub(ecoPath);
        hub.setPreferences("yatharth_dev", "Master Trees & Graphs");
        hub.toggleRoadmapTopic("arrays", true);
        hub.updatePracticeProblem("p_two_sum", "toggle_solve", "", "");
        hub.updatePracticeProblem("p_two_sum", "save_notes", "Line 1\nLine 2 | pipe", "HashMap O(n)");
        auto lc = hub.addLeetCodeProblem("Two Sum", "https://leetcode.com/problems/two-sum/",
                                         "Easy", "Hash Tables", "Solved", "Good warmup");
        hub.updateLeetCodeProblem(lc.id, "Needs Revision", true, false, false, false, "", "", "");
        hub.reviewFlashcard("fc_1", "good");
        hub.addMistake("Off-by-one Binary Search", "Searching", "Check low <= high");
    }

    {
        LearningHub reloaded(ecoPath);
        assert(reloaded.preferences().leetcodeUsername == "yatharth_dev");
        assert(reloaded.preferences().learningGoal == "Master Trees & Graphs");
        bool arraysDone = false;
        for (const auto& t : reloaded.roadmap()) {
            if (t.id == "arrays") arraysDone = t.completed;
        }
        assert(arraysDone);
        bool twoSumNotes = false;
        for (const auto& p : reloaded.practiceProblems()) {
            if (p.id == "p_two_sum") {
                assert(p.solved && p.notes.find("| pipe") != std::string::npos);
                twoSumNotes = true;
            }
        }
        assert(twoSumNotes);
        assert(reloaded.leetcodeProblems().size() == 1);
        assert(reloaded.leetcodeProblems()[0].bookmarked);
        assert(reloaded.leetcodeProblems()[0].status == "Needs Revision");
        assert(reloaded.mistakes().size() == 1);

        ProgressManager pm(progPath);
        HistoryLogger logger(logPath);
        auto analytics = reloaded.computeAnalytics(pm, logger);
        assert(analytics.problemsSolved == 1);
        assert(analytics.roadmapCompleted == 1);
        assert(analytics.revisionTotalReviews == 1);
        assert(analytics.weeklyActivity.size() == 7);
        assert(analytics.dailyHeatmap.size() == 28);
    }

    for (const auto& p : {ecoPath, progPath, logPath}) {
        std::filesystem::remove(p);
    }
}

void testIntelligenceHub() {
    const std::string catPath = "/tmp/dsa_intel_cat_test.dat";
    const std::string statePath = "/tmp/dsa_intel_state_test.dat";
    std::filesystem::remove(catPath);
    std::filesystem::remove(statePath);

    const std::string sampleCatalog = R"({
      "questions": [
        {"frontendQuestionId": "1", "title": "Two Sum", "titleSlug": "two-sum", "difficulty": "Easy", "paidOnly": false, "topicTags": [{"name": "Array"}, {"name": "Hash Table"}]},
        {"frontendQuestionId": "2", "title": "Add Two Numbers", "titleSlug": "add-two-numbers", "difficulty": "Medium", "paidOnly": false, "topicTags": [{"name": "Linked List"}]},
        {"frontendQuestionId": "53", "title": "Maximum Subarray", "titleSlug": "maximum-subarray", "difficulty": "Medium", "paidOnly": false, "topicTags": [{"name": "Array"}, {"name": "Dynamic Programming"}]},
        {"frontendQuestionId": "42", "title": "Trapping Rain Water", "titleSlug": "trapping-rain-water", "difficulty": "Hard", "paidOnly": false, "topicTags": [{"name": "Array"}, {"name": "Two Pointers"}]},
        {"frontendQuestionId": "157", "title": "Read N Characters", "titleSlug": "read-n-characters", "difficulty": "Easy", "paidOnly": true, "topicTags": [{"name": "String"}]}
      ]
    })";

    {
        IntelligenceHub hub(catPath, statePath);
        assert(hub.importCatalogueJson(sampleCatalog) == 5);
        // Re-importing same questions must add 0 duplicates
        assert(hub.importCatalogueJson(sampleCatalog) == 0);

        CatalogueQuery q;
        q.availability = "free";
        q.topic = "Array";
        auto page = hub.queryCatalogue(q);
        assert(page.totalFiltered == 3 && page.totalCatalogue == 5);

        // Solve Two Sum -> Array (1/3 = 33.3%) and Hash Table (1/1) updated automatically
        hub.updateProblemStatus("two-sum", "Solved", true, true, false, false, false);
        auto topics = hub.computeTopicProgress();
        bool checkedArray = false;
        for (const auto& t : topics) {
            if (t.topic == "Array") {
                assert(t.totalProblems == 3 && t.solvedProblems == 1 && t.remainingProblems == 2);
                assert(t.completionPercent < 50.0); // Not falsely 100%!
                checkedArray = true;
            }
        }
        assert(checkedArray);

        // Notes & multi-approach solutions
        ProblemNotes notes;
        notes.slug = "two-sum";
        notes.intuition = "Use complement map";
        notes.cppSnippet = "unordered_map<int,int> seen;\n// check target - x | pipe";
        hub.saveProblemNotes(notes);

        SavedSolution sol1;
        sol1.slug = "two-sum";
        sol1.approachName = "Brute Force";
        sol1.timeComplexity = "O(n^2)";
        sol1.spaceComplexity = "O(1)";
        sol1.cppCode = "for (int i=0;i<n;++i) {}";
        hub.addSolution(sol1);

        SavedSolution sol2;
        sol2.slug = "two-sum";
        sol2.approachName = "Hash Map";
        sol2.timeComplexity = "O(n)";
        sol2.spaceComplexity = "O(n)";
        sol2.cppCode = "unordered_map<int,int> mp;";
        hub.addSolution(sol2);

        assert(hub.searchSolutions("two-sum", "all", "all").size() == 2);
        assert(hub.searchSolutions("two-sum", "all", "O(n)").size() == 1);

        // Extension submission & deduplication
        hub.recordExtensionSubmission(53, "Maximum Subarray", "maximum-subarray", "Medium", "2026-11-25 12:00:00");
        hub.recordExtensionSubmission(53, "Maximum Subarray", "maximum-subarray", "Medium", "2026-11-25 12:00:00");
        assert(hub.recentSubmissions(10).size() == 1);

        // Revision scheduling
        hub.scheduleProblemRevision("trapping-rain-water", true);
        hub.reviewProblemRevision("trapping-rain-water", "good");
        assert(hub.revisionQueue().size() == 1);
    }

    // Reload from disk and verify persistence
    {
        IntelligenceHub reloaded(catPath, statePath);
        auto detail = reloaded.getProblemDetail("two-sum");
        assert(detail.progress.status == "Solved");
        assert(detail.progress.bookmarked);
        assert(detail.notes.intuition == "Use complement map");
        assert(detail.notes.cppSnippet.find("| pipe") != std::string::npos);
        assert(reloaded.searchNotes("complement").size() == 1);
        assert(detail.solutions.size() == 2);
        auto an = reloaded.computeAnalytics();
        assert(an.totalCatalogue == 5);
        assert(an.totalSolved == 2);
        assert(an.solvedEasy == 1 && an.solvedMedium == 1);
        assert(an.currentStreak >= 1);
    }

    std::filesystem::remove(catPath);
    std::filesystem::remove(statePath);
}

} // namespace

int main() {
    testTemplatesAndOverloads();
    testSortingAlgorithms();
    testSearchingAlgorithms();
    testStackAndQueue();
    testHistoryLogger();
    testComparator();
    testBinarySearchTree();
    testGraph();
    testQuizAndProgress();
    testLearningHubPersistence();
    testIntelligenceHub();
    std::cout << "All DSA Manager Lite C++ unit tests passed!\n";
    return 0;
}
