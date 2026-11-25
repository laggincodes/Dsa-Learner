#include "dsa/LearningHub.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace dsa {

LearningHub::LearningHub(std::string filePath) : filePath_(std::move(filePath)) {
    initDefaults();
    loadFromDisk();
}

std::string LearningHub::todayDate() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
    localtime_r(&t, &tmBuf);
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d");
    return oss.str();
}

std::string LearningHub::addDaysToToday(int days) {
    const auto target = std::chrono::system_clock::now() + std::chrono::hours(24 * days);
    const std::time_t t = std::chrono::system_clock::to_time_t(target);
    std::tm tmBuf{};
    localtime_r(&t, &tmBuf);
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d");
    return oss.str();
}

std::string LearningHub::escapeField(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '|') out += "\\p";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "";
        else out.push_back(c);
    }
    return out;
}

std::string LearningHub::unescapeField(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char next = s[i + 1];
            if (next == '\\') { out.push_back('\\'); ++i; }
            else if (next == 'p') { out.push_back('|'); ++i; }
            else if (next == 'n') { out.push_back('\n'); ++i; }
            else { out.push_back(s[i]); }
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

std::vector<std::string> LearningHub::splitEscaped(const std::string& line) {
    std::vector<std::string> parts;
    std::string curr;
    for (char c : line) {
        if (c == '|') {
            parts.push_back(unescapeField(curr));
            curr.clear();
        } else {
            curr.push_back(c);
        }
    }
    parts.push_back(unescapeField(curr));
    return parts;
}

void LearningHub::initDefaults() {
    roadmap_ = {
        // FOUNDATIONS
        {"complexity", "FOUNDATIONS", "Time and Space Complexity",
         "Asymptotic Big-O notation, growth rates, best/average/worst bounds, and auxiliary memory analysis.",
         "Beginner", {}, "comparator", {"p_two_sum", "p_max_subarray"}},
        {"arrays", "FOUNDATIONS", "Arrays",
         "Contiguous memory indexing, prefix sums, two-pointer scans, and sliding window patterns.",
         "Beginner", {"Time and Space Complexity"}, "sorting", {"p_two_sum", "p_max_subarray", "p_rotate_array"}},
        {"strings", "FOUNDATIONS", "Strings",
         "Character frequency maps, palindrome checks, anagrams, and substring sliding windows.",
         "Beginner", {"Arrays"}, "searching", {"p_valid_anagram", "p_longest_substr"}},
        {"searching", "FOUNDATIONS", "Searching",
         "Sequential scan vs monotonic Binary Search and binary search on answer spaces.",
         "Beginner", {"Arrays"}, "searching", {"p_binary_search", "p_rotated_search"}},
        {"sorting", "FOUNDATIONS", "Sorting",
         "Comparison sorts (Bubble, Selection, Insertion), stability, in-place swaps, and divide-and-conquer.",
         "Beginner", {"Arrays"}, "sorting", {"p_merge_intervals"}},

        // DATA STRUCTURES
        {"linked_lists", "DATA STRUCTURES", "Linked Lists",
         "Pointer-linked nodes supporting O(1) head insertion, reversal, and fast/slow cycle detection.",
         "Beginner", {"Arrays"}, "datastructures:list", {"p_reverse_list", "p_cycle_list"}},
        {"stacks", "DATA STRUCTURES", "Stacks",
         "LIFO containers powering expression parsing, balanced parentheses, and monotonic stack patterns.",
         "Beginner", {"Arrays"}, "datastructures:stack", {"p_valid_parens", "p_daily_temps"}},
        {"queues", "DATA STRUCTURES", "Queues",
         "FIFO buffers, circular queues, deque sliding windows, and level-order traversal scheduling.",
         "Beginner", {"Linked Lists"}, "datastructures:queue", {"p_queue_using_stacks"}},
        {"hash_tables", "DATA STRUCTURES", "Hash Tables",
         "Key-value lookup in amortized O(1) time via hash functions and collision resolution.",
         "Intermediate", {"Arrays"}, "codeviewer", {"p_two_sum", "p_group_anagrams"}},
        {"trees", "DATA STRUCTURES", "Trees",
         "Hierarchical parent-child nodes, DFS traversals (preorder, inorder, postorder), and BFS levels.",
         "Intermediate", {"Queues", "Recursion"}, "datastructures:bst", {"p_max_depth_tree"}},
        {"bst", "DATA STRUCTURES", "Binary Search Trees",
         "Ordered binary trees preserving left < root < right for logarithmic average search and deletion.",
         "Intermediate", {"Trees"}, "datastructures:bst", {"p_validate_bst"}},
        {"heaps", "DATA STRUCTURES", "Heaps",
         "Complete binary trees maintaining min/max priority invariants for O(log n) push/pop and top-k queries.",
         "Intermediate", {"Trees"}, "codeviewer", {"p_kth_largest"}},
        {"graphs", "DATA STRUCTURES", "Graphs",
         "Adjacency lists and matrices representing directed/undirected networks, cycles, and components.",
         "Intermediate", {"Queues", "Stacks"}, "graph", {"p_num_islands"}},

        // ALGORITHMS
        {"recursion", "ALGORITHMS", "Recursion",
         "Self-referential subproblems, base cases, call-stack frames, and recurrence tree analysis.",
         "Beginner", {"Stacks"}, "datastructures:bst", {"p_subsets"}},
        {"backtracking", "ALGORITHMS", "Backtracking",
         "Incremental state-space search with pruning to enumerate permutations, combinations, and constraints.",
         "Intermediate", {"Recursion"}, "codeviewer", {"p_subsets", "p_n_queens"}},
        {"greedy", "ALGORITHMS", "Greedy Algorithms",
         "Locally optimal choices yielding global optima for intervals, scheduling, and Huffman/MST problems.",
         "Intermediate", {"Sorting"}, "sorting", {"p_merge_intervals"}},
        {"dp", "ALGORITHMS", "Dynamic Programming",
         "Optimal substructure and overlapping subproblems solved via memoization or bottom-up tabulation.",
         "Advanced", {"Recursion"}, "codeviewer", {"p_coin_change", "p_lis"}},
        {"graph_algos", "ALGORITHMS", "Graph Algorithms",
         "BFS shortest unweighted paths, DFS reachability, topological sort, and cycle detection.",
         "Advanced", {"Graphs"}, "graph", {"p_num_islands", "p_course_schedule"}}
    };

    practice_ = {
        {"p_two_sum", "Two Sum", "Hash Tables", "Easy",
         "Given an integer array nums and an integer target, return indices of two numbers adding to target.",
         "Store seen value -> index in an unordered_map as you scan left to right.",
         "Single-pass hash map lookup: O(n) time, O(n) space."},
        {"p_max_subarray", "Maximum Subarray (Kadane)", "Arrays", "Medium",
         "Find the contiguous subarray within an array of integers that has the largest sum.",
         "At each index, choose between extending the current subarray or starting fresh at nums[i].",
         "Kadane's DP scan: current = max(nums[i], current + nums[i]); O(n) time, O(1) space."},
        {"p_rotate_array", "Rotate Array by K", "Arrays", "Medium",
         "Rotate an array of n elements to the right by k steps in-place.",
         "Reverse the whole array, reverse first k elements, then reverse remaining n - k elements.",
         "Three-reversal technique: O(n) time, O(1) space."},
        {"p_valid_anagram", "Valid Anagram", "Strings", "Easy",
         "Determine whether string t is an anagram of string s.",
         "Count character frequencies with a fixed 26-slot array.",
         "Increment for s, decrement for t, verify all zero: O(n) time, O(1) space."},
        {"p_longest_substr", "Longest Substring Without Repeating Characters", "Strings", "Medium",
         "Find the length of the longest substring without duplicate characters.",
         "Maintain a sliding window [left, right] and last-seen index array.",
         "Sliding window with jump pointer: O(n) time, O(1) alphabet space."},
        {"p_binary_search", "Binary Search", "Searching", "Easy",
         "Find target in a sorted ascending array in O(log n) time or return -1.",
         "Use mid = low + (high - low) / 2 to avoid integer overflow.",
         "Iterative interval halving: O(log n) time, O(1) space."},
        {"p_rotated_search", "Search in Rotated Sorted Array", "Searching", "Medium",
         "Search target in a sorted array rotated at an unknown pivot in O(log n).",
         "At every mid, at least one half [low..mid] or [mid..high] is strictly sorted.",
         "Modified binary search checking sorted half bounds: O(log n) time."},
        {"p_merge_intervals", "Merge Intervals", "Sorting", "Medium",
         "Merge all overlapping [start, end] intervals and return non-overlapping intervals.",
         "Sort intervals by start time first, then merge sequentially.",
         "Sort O(n log n) + linear sweep O(n)."},
        {"p_reverse_list", "Reverse Linked List", "Linked Lists", "Easy",
         "Reverse a singly linked list and return the new head.",
         "Track prev, curr, and next pointers iteratively.",
         "Three-pointer in-place reversal: O(n) time, O(1) space."},
        {"p_cycle_list", "Linked List Cycle Detection", "Linked Lists", "Easy",
         "Determine if a linked list contains a cycle.",
         "Advance slow by 1 step and fast by 2 steps.",
         "Floyd's Tortoise and Hare: O(n) time, O(1) space."},
        {"p_valid_parens", "Valid Parentheses", "Stacks", "Easy",
         "Check if brackets (), {}, [] are properly nested and matched.",
         "Push opening brackets on a stack and match each closing bracket with stack.top().",
         "Stack matching: O(n) time, O(n) space."},
        {"p_daily_temps", "Daily Temperatures", "Stacks", "Medium",
         "For each day, compute how many days until a warmer temperature occurs.",
         "Keep a monotonically decreasing stack of unresolved day indices.",
         "Monotonic stack: O(n) amortized time, O(n) space."},
        {"p_queue_using_stacks", "Implement Queue using Two Stacks", "Queues", "Easy",
         "Implement FIFO push, pop, peek using only LIFO stacks.",
         "Use an input stack and lazy-transfer output stack.",
         "Amortized O(1) per operation."},
        {"p_group_anagrams", "Group Anagrams", "Hash Tables", "Medium",
         "Group an array of strings into anagram equivalence buckets.",
         "Key each string by its sorted form or 26-char count signature.",
         "Hash map grouping: O(n * k log k) or O(n * k)."},
        {"p_max_depth_tree", "Maximum Depth of Binary Tree", "Trees", "Easy",
         "Return the longest root-to-leaf path length in a binary tree.",
         "Recursively compute 1 + max(depth(left), depth(right)).",
         "DFS recursion or BFS level count: O(n) time."},
        {"p_validate_bst", "Validate Binary Search Tree", "Binary Search Trees", "Medium",
         "Verify every node strictly satisfies minBound < node.val < maxBound.",
         "Pass (min, max) bounds down DFS or verify strictly increasing inorder traversal.",
         "Bounded DFS: O(n) time, O(h) stack space."},
        {"p_kth_largest", "Kth Largest Element in Array", "Heaps", "Medium",
         "Find the kth largest element in an unsorted integer array.",
         "Maintain a min-heap of size k; the root is the kth largest.",
         "Min-heap priority_queue: O(n log k) time, O(k) space."},
        {"p_num_islands", "Number of Islands", "Graphs", "Medium",
         "Count connected components of '1's in a 2D grid using 4-directional adjacency.",
         "Trigger BFS/DFS from each unvisited land cell and mark visited.",
         "Grid BFS/DFS flood fill: O(R * C) time."},
        {"p_subsets", "Subsets (Power Set)", "Backtracking", "Medium",
         "Generate all 2^n subsets of distinct integers.",
         "At each index, branch on including or excluding nums[i].",
         "Backtracking DFS: O(n * 2^n) time."},
        {"p_n_queens", "N-Queens", "Backtracking", "Hard",
         "Place N queens on an N x N board so no two attack each other.",
         "Track used columns and both diagonals (r - c, r + c) in O(1).",
         "Row-by-row backtracking with diagonal bitsets/arrays."},
        {"p_coin_change", "Coin Change (Min Coins)", "Dynamic Programming", "Medium",
         "Compute minimum coins needed to sum to amount, or -1 if impossible.",
         "dp[a] = min(dp[a], 1 + dp[a - coin]) for every valid coin.",
         "1D bottom-up DP: O(amount * coins) time, O(amount) space."},
        {"p_lis", "Longest Increasing Subsequence", "Dynamic Programming", "Medium",
         "Return the length of the longest strictly increasing subsequence.",
         "Use dp[i] = 1 + max(dp[j]) for j < i with nums[j] < nums[i], or patience sorting.",
         "O(n^2) DP or O(n log n) binary search tails."},
        {"p_course_schedule", "Course Schedule (Cycle Check)", "Graph Algorithms", "Medium",
         "Determine if all courses can be finished given prerequisite pairs.",
         "Build directed graph and run Kahn's BFS topological sort or 3-color DFS.",
         "Topological sort: O(V + E) time and space."}
    };

    flashcards_ = {
        {"fc_1", "Complexity", "What is the time & space complexity of Binary Search?",
         "Time: O(1) best, O(log n) average/worst. Space: O(1) iterative, O(log n) recursive stack. Requires sorted input.",
         0, 2.5, 0, todayDate(), "New", 0},
        {"fc_2", "Complexity", "Compare Bubble Sort, Selection Sort, and Insertion Sort swaps.",
         "Selection Sort performs at most (n - 1) swaps. Insertion Sort shifts only inversions (O(n) on sorted input). Bubble Sort can perform up to O(n^2) swaps.",
         0, 2.5, 0, todayDate(), "New", 0},
        {"fc_3", "Concept", "Why does Inorder traversal of a BST yield sorted output?",
         "Because every BST node enforces Left < Node < Right, visiting Left subtree then Node then Right subtree visits keys in non-decreasing order.",
         0, 2.5, 0, todayDate(), "New", 0},
        {"fc_4", "Algorithm", "How do you delete a BST node with two children?",
         "Replace its key with its inorder successor (minimum node in right subtree) or inorder predecessor, then recursively delete that successor.",
         0, 2.5, 0, todayDate(), "New", 0},
        {"fc_5", "Algorithm", "When should you prefer BFS over DFS on an unweighted graph?",
         "Use BFS when you need the shortest path by number of edges from a source, or level-by-level exploration.",
         0, 2.5, 0, todayDate(), "New", 0},
        {"fc_6", "Complexity", "What are the time and space complexities of BFS and DFS (Adjacency List)?",
         "Time: O(V + E) because each vertex and edge is inspected once/twice. Auxiliary Space: O(V) for visited set and queue/stack.",
         0, 2.5, 0, todayDate(), "New", 0},
        {"fc_7", "Concept", "What distinguishes Dynamic Programming from Divide and Conquer?",
         "Dynamic Programming solves problems with overlapping subproblems by caching results, whereas Divide and Conquer (like Merge Sort) solves independent subproblems.",
         0, 2.5, 0, todayDate(), "New", 0},
        {"fc_8", "Concept", "What is the difference between Stack Overflow and Stack Underflow?",
         "Overflow occurs when pushing onto a full bounded stack (or exhausting call stack). Underflow occurs when popping/peeking an empty stack.",
         0, 2.5, 0, todayDate(), "New", 0}
    };

    resources_ = {
        {"res_leetcode", "LeetCode", "Practice Platform",
         "Curated coding interview problems, company tags, and weekly contests.",
         "https://leetcode.com/problemset/"},
        {"res_gfg", "GeeksforGeeks", "Tutorials & Articles",
         "Topic-wise DSA explanations, walkthroughs, and complexity breakdowns.",
         "https://www.geeksforgeeks.org/data-structures/"},
        {"res_cf", "Codeforces", "Competitive Programming",
         "Rated algorithmic contests, problem ratings, and community editorials.",
         "https://codeforces.com/problemset"},
        {"res_cc", "CodeChef", "Practice & Contests",
         "Structured beginner-to-advanced DSA practice tracks and monthly contests.",
         "https://www.codechef.com/practice"},
        {"res_cp", "CP-Algorithms", "Algorithm Reference",
         "Rigorous mathematical proofs and clean C++ implementations for classic algorithms.",
         "https://cp-algorithms.com/"},
        {"res_cpp", "C++ Reference", "Language & STL Docs",
         "Authoritative documentation for C++17 containers, iterators, and <algorithm>.",
         "https://en.cppreference.com/w/"}
    };
}

void LearningHub::loadFromDisk() {
    std::ifstream in(filePath_);
    if (!in.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto parts = splitEscaped(line);
        if (parts.empty()) continue;
        const std::string& tag = parts[0];

        if (tag == "PREF" && parts.size() >= 3) {
            prefs_.leetcodeUsername = parts[1];
            prefs_.learningGoal = parts[2];
        } else if (tag == "ROADMAP" && parts.size() >= 3) {
            for (auto& t : roadmap_) {
                if (t.id == parts[1]) {
                    t.completed = (parts[2] == "1");
                    break;
                }
            }
        } else if (tag == "PRACTICE" && parts.size() >= 7) {
            for (auto& p : practice_) {
                if (p.id == parts[1]) {
                    p.solved = (parts[2] == "1");
                    p.bookmarked = (parts[3] == "1");
                    p.needsRevision = (parts[4] == "1");
                    try { p.attempts = static_cast<std::size_t>(std::stoul(parts[5])); } catch (...) {}
                    p.notes = parts[6];
                    if (parts.size() >= 8) p.userApproach = parts[7];
                    break;
                }
            }
        } else if (tag == "LEETCODE" && parts.size() >= 10) {
            LeetCodeProblem lp;
            lp.id = parts[1];
            lp.title = parts[2];
            lp.url = parts[3];
            lp.difficulty = parts[4];
            lp.topic = parts[5];
            lp.status = parts[6];
            lp.bookmarked = (parts[7] == "1");
            lp.notes = parts[8];
            lp.updatedAt = parts[9];
            leetcode_.push_back(std::move(lp));
            if (parts[1].rfind("lc-", 0) == 0) {
                try {
                    std::size_t seq = static_cast<std::size_t>(std::stoul(parts[1].substr(3)));
                    if (seq >= nextLcSeq_) nextLcSeq_ = seq + 1;
                } catch (...) {}
            }
        } else if (tag == "FLASHCARD" && parts.size() >= 7) {
            for (auto& fc : flashcards_) {
                if (fc.id == parts[1]) {
                    try {
                        fc.intervalDays = std::stoi(parts[2]);
                        fc.easeFactor = std::stod(parts[3]);
                        fc.repetitions = std::stoi(parts[4]);
                        fc.nextReviewDate = parts[5];
                        fc.lastRating = parts[6];
                        if (parts.size() >= 8) fc.reviewCount = static_cast<std::size_t>(std::stoul(parts[7]));
                    } catch (...) {}
                    break;
                }
            }
        } else if (tag == "MISTAKE" && parts.size() >= 6) {
            MistakeNote m;
            m.id = parts[1];
            m.title = parts[2];
            m.topic = parts[3];
            m.note = parts[4];
            m.createdAt = parts[5];
            mistakes_.push_back(std::move(m));
            if (parts[1].rfind("mk-", 0) == 0) {
                try {
                    std::size_t seq = static_cast<std::size_t>(std::stoul(parts[1].substr(3)));
                    if (seq >= nextMistakeSeq_) nextMistakeSeq_ = seq + 1;
                } catch (...) {}
            }
        } else if (tag == "DAY" && parts.size() >= 3) {
            try {
                dailyActivity_[parts[1]] = static_cast<std::size_t>(std::stoul(parts[2]));
            } catch (...) {}
        }
    }
}

void LearningHub::saveToDisk() const {
    std::ofstream out(filePath_, std::ios::trunc);
    if (!out.is_open()) {
        throw FileIOException("Failed to write ecosystem data file: " + filePath_);
    }

    out << "PREF|" << escapeField(prefs_.leetcodeUsername) << "|" << escapeField(prefs_.learningGoal) << "\n";

    for (const auto& t : roadmap_) {
        out << "ROADMAP|" << escapeField(t.id) << "|" << (t.completed ? "1" : "0") << "\n";
    }
    for (const auto& p : practice_) {
        out << "PRACTICE|" << escapeField(p.id) << "|"
            << (p.solved ? "1" : "0") << "|"
            << (p.bookmarked ? "1" : "0") << "|"
            << (p.needsRevision ? "1" : "0") << "|"
            << p.attempts << "|"
            << escapeField(p.notes) << "|"
            << escapeField(p.userApproach) << "\n";
    }
    for (const auto& lp : leetcode_) {
        out << "LEETCODE|" << escapeField(lp.id) << "|"
            << escapeField(lp.title) << "|"
            << escapeField(lp.url) << "|"
            << escapeField(lp.difficulty) << "|"
            << escapeField(lp.topic) << "|"
            << escapeField(lp.status) << "|"
            << (lp.bookmarked ? "1" : "0") << "|"
            << escapeField(lp.notes) << "|"
            << escapeField(lp.updatedAt) << "\n";
    }
    for (const auto& fc : flashcards_) {
        out << "FLASHCARD|" << escapeField(fc.id) << "|"
            << fc.intervalDays << "|"
            << fc.easeFactor << "|"
            << fc.repetitions << "|"
            << escapeField(fc.nextReviewDate) << "|"
            << escapeField(fc.lastRating) << "|"
            << fc.reviewCount << "\n";
    }
    for (const auto& m : mistakes_) {
        out << "MISTAKE|" << escapeField(m.id) << "|"
            << escapeField(m.title) << "|"
            << escapeField(m.topic) << "|"
            << escapeField(m.note) << "|"
            << escapeField(m.createdAt) << "\n";
    }
    for (const auto& [day, count] : dailyActivity_) {
        out << "DAY|" << escapeField(day) << "|" << count << "\n";
    }
    out.flush();
}

void LearningHub::recordActivityToday(std::size_t units) {
    std::lock_guard<std::mutex> lock(mutex_);
    dailyActivity_[todayDate()] += units;
    saveToDisk();
}

void LearningHub::setPreferences(const std::string& leetcodeUsername, const std::string& learningGoal) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (leetcodeUsername.size() > 64) {
        throw ValidationException("LeetCode username must be 64 characters or fewer.");
    }
    if (learningGoal.size() > 160) {
        throw ValidationException("Learning goal must be 160 characters or fewer.");
    }
    for (char c : leetcodeUsername) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') {
            throw ValidationException("LeetCode username may only contain letters, digits, '-' or '_'.");
        }
    }
    prefs_.leetcodeUsername = leetcodeUsername;
    if (!learningGoal.empty()) {
        prefs_.learningGoal = learningGoal;
    }
    saveToDisk();
}

UserPreferences LearningHub::preferences() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return prefs_;
}

void LearningHub::toggleRoadmapTopic(const std::string& topicId, bool completed) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool found = false;
    for (auto& t : roadmap_) {
        if (t.id == topicId) {
            t.completed = completed;
            found = true;
            break;
        }
    }
    if (!found) {
        throw ValidationException("Unknown roadmap topic ID: '" + topicId + "'.");
    }
    if (completed) {
        dailyActivity_[todayDate()] += 1;
    }
    saveToDisk();
}

std::vector<RoadmapTopic> LearningHub::roadmap() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return roadmap_;
}

void LearningHub::updatePracticeProblem(const std::string& problemId, const std::string& action,
                                        const std::string& notes, const std::string& approach) {
    std::lock_guard<std::mutex> lock(mutex_);
    PracticeProblem* target = nullptr;
    for (auto& p : practice_) {
        if (p.id == problemId) {
            target = &p;
            break;
        }
    }
    if (!target) {
        throw ValidationException("Unknown practice problem ID: '" + problemId + "'.");
    }

    if (action == "toggle_solve") {
        target->solved = !target->solved;
        if (target->solved) {
            ++target->attempts;
            dailyActivity_[todayDate()] += 1;
        }
    } else if (action == "attempt") {
        ++target->attempts;
        dailyActivity_[todayDate()] += 1;
    } else if (action == "toggle_bookmark") {
        target->bookmarked = !target->bookmarked;
    } else if (action == "toggle_revision") {
        target->needsRevision = !target->needsRevision;
    } else if (action == "save_notes") {
        target->notes = notes;
        target->userApproach = approach;
    } else {
        throw ValidationException("Unknown practice action: '" + action + "'.");
    }
    saveToDisk();
}

std::vector<PracticeProblem> LearningHub::practiceProblems() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return practice_;
}

LeetCodeProblem LearningHub::addLeetCodeProblem(const std::string& title, const std::string& url,
                                                const std::string& difficulty, const std::string& topic,
                                                const std::string& status, const std::string& notes) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (title.empty() || title.size() > 120) {
        throw ValidationException("Problem title must be 1 to 120 characters.");
    }
    if (!url.empty() && url.rfind("https://", 0) != 0 && url.rfind("http://", 0) != 0) {
        throw ValidationException("Problem URL must start with https:// or http://");
    }
    if (difficulty != "Easy" && difficulty != "Medium" && difficulty != "Hard") {
        throw ValidationException("Difficulty must be Easy, Medium, or Hard.");
    }
    if (status != "Not Started" && status != "Attempted" && status != "Solved" && status != "Needs Revision") {
        throw ValidationException("Invalid problem status.");
    }

    LeetCodeProblem lp;
    lp.id = "lc-" + std::to_string(nextLcSeq_++);
    lp.title = title;
    lp.url = url.empty() ? ("https://leetcode.com/problemset/?search=" + title) : url;
    lp.difficulty = difficulty;
    lp.topic = topic.empty() ? "Arrays" : topic;
    lp.status = status;
    lp.notes = notes;
    lp.updatedAt = todayDate();

    leetcode_.push_back(lp);
    if (status == "Solved" || status == "Attempted") {
        dailyActivity_[todayDate()] += 1;
    }
    saveToDisk();
    return lp;
}

LeetCodeProblem LearningHub::updateLeetCodeProblem(const std::string& id, const std::string& status,
                                                   bool toggleBookmark, bool hasBookmark, bool bookmarkVal,
                                                   bool hasNotes, const std::string& notes,
                                                   const std::string& difficulty, const std::string& topic) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& lp : leetcode_) {
        if (lp.id == id) {
            if (!status.empty()) {
                if (status != "Not Started" && status != "Attempted" && status != "Solved" && status != "Needs Revision") {
                    throw ValidationException("Invalid problem status.");
                }
                if (lp.status != status && (status == "Solved" || status == "Attempted")) {
                    dailyActivity_[todayDate()] += 1;
                }
                lp.status = status;
            }
            if (toggleBookmark) {
                lp.bookmarked = !lp.bookmarked;
            } else if (hasBookmark) {
                lp.bookmarked = bookmarkVal;
            }
            if (hasNotes) {
                lp.notes = notes;
            }
            if (!difficulty.empty()) lp.difficulty = difficulty;
            if (!topic.empty()) lp.topic = topic;
            lp.updatedAt = todayDate();
            saveToDisk();
            return lp;
        }
    }
    throw ValidationException("LeetCode problem ID not found: '" + id + "'.");
}

void LearningHub::deleteLeetCodeProblem(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(leetcode_.begin(), leetcode_.end(), [&](const LeetCodeProblem& lp) {
        return lp.id == id;
    });
    if (it == leetcode_.end()) {
        throw ValidationException("LeetCode problem ID not found.");
    }
    leetcode_.erase(it, leetcode_.end());
    saveToDisk();
}

std::vector<LeetCodeProblem> LearningHub::leetcodeProblems() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return leetcode_;
}

RevisionCard LearningHub::reviewFlashcard(const std::string& cardId, const std::string& rating) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& fc : flashcards_) {
        if (fc.id == cardId) {
            // Simple SM-2-inspired local spaced repetition scheduler
            if (rating == "again") {
                fc.repetitions = 0;
                fc.intervalDays = 0;
                fc.easeFactor = std::max(1.3, fc.easeFactor - 0.2);
                fc.lastRating = "Again";
            } else if (rating == "difficult") {
                fc.repetitions = std::max(1, fc.repetitions);
                fc.intervalDays = std::max(1, fc.intervalDays == 0 ? 1 : static_cast<int>(fc.intervalDays * 1.2));
                fc.easeFactor = std::max(1.3, fc.easeFactor - 0.1);
                fc.lastRating = "Difficult";
            } else if (rating == "good") {
                ++fc.repetitions;
                if (fc.repetitions == 1) fc.intervalDays = 1;
                else if (fc.repetitions == 2) fc.intervalDays = 3;
                else fc.intervalDays = static_cast<int>(std::max(3.0, fc.intervalDays * fc.easeFactor));
                fc.lastRating = "Good";
            } else if (rating == "easy") {
                ++fc.repetitions;
                if (fc.repetitions == 1) fc.intervalDays = 4;
                else fc.intervalDays = static_cast<int>(std::max(5.0, fc.intervalDays * (fc.easeFactor + 0.3)));
                fc.easeFactor = std::min(3.0, fc.easeFactor + 0.15);
                fc.lastRating = "Easy";
            } else {
                throw ValidationException("Invalid rating. Use again, difficult, good, or easy.");
            }

            fc.nextReviewDate = addDaysToToday(fc.intervalDays);
            ++fc.reviewCount;
            dailyActivity_[todayDate()] += 1;
            saveToDisk();
            return fc;
        }
    }
    throw ValidationException("Flashcard ID not found.");
}

std::vector<RevisionCard> LearningHub::flashcards() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return flashcards_;
}

MistakeNote LearningHub::addMistake(const std::string& title, const std::string& topic, const std::string& note) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (title.empty() || note.empty()) {
        throw ValidationException("Mistake title and note are required.");
    }
    MistakeNote m{"mk-" + std::to_string(nextMistakeSeq_++), title, topic.empty() ? "General" : topic, note, todayDate()};
    mistakes_.push_back(m);
    dailyActivity_[todayDate()] += 1;
    saveToDisk();
    return m;
}

void LearningHub::removeMistake(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(mistakes_.begin(), mistakes_.end(), [&](const MistakeNote& m) {
        return m.id == id;
    });
    if (it == mistakes_.end()) {
        throw ValidationException("Mistake entry not found.");
    }
    mistakes_.erase(it, mistakes_.end());
    saveToDisk();
}

std::vector<MistakeNote> LearningHub::mistakes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return mistakes_;
}

std::vector<ExternalResource> LearningHub::resources() const {
    return resources_;
}

AnalyticsSnapshot LearningHub::computeAnalytics(const ProgressManager& progress,
                                                const HistoryLogger& logger) const {
    std::lock_guard<std::mutex> lock(mutex_);
    AnalyticsSnapshot snap;

    std::map<std::string, std::size_t> topicCounts;
    for (const auto& p : practice_) {
        if (p.solved) ++snap.problemsSolved;
        if (p.attempts > 0 || p.solved) {
            ++snap.problemsAttempted;
            topicCounts[p.topic] += std::max<std::size_t>(1, p.attempts);
        }
        if (p.needsRevision) ++snap.needsRevisionCount;
    }

    snap.leetcodeTotal = leetcode_.size();
    for (const auto& lp : leetcode_) {
        if (lp.status == "Solved") {
            ++snap.leetcodeSolved;
            ++snap.problemsSolved;
        }
        if (lp.status != "Not Started") {
            ++snap.problemsAttempted;
            topicCounts[lp.topic] += 1;
        }
        if (lp.status == "Needs Revision") {
            ++snap.needsRevisionCount;
        }
    }

    snap.roadmapTotal = roadmap_.size();
    for (const auto& t : roadmap_) {
        if (t.completed) {
            ++snap.roadmapCompleted;
            topicCounts[t.title] += 1;
        }
    }
    snap.topicCompletionPercent = snap.roadmapTotal > 0
        ? (100.0 * static_cast<double>(snap.roadmapCompleted) / static_cast<double>(snap.roadmapTotal))
        : 0.0;

    const auto progSnap = progress.snapshot();
    snap.quizAttempts = progSnap.totalQuizzesAttempted;
    snap.quizAvgAccuracy = progSnap.averageAccuracy;

    const std::string today = todayDate();
    for (const auto& fc : flashcards_) {
        snap.revisionTotalReviews += fc.reviewCount;
        if (fc.nextReviewDate.empty() || fc.nextReviewDate <= today) {
            ++snap.revisionDueCount;
        }
    }
    snap.mistakesCount = mistakes_.size();

    // Merge real daily activity from ecosystem + history log timestamps
    std::map<std::string, std::size_t> combinedDays = dailyActivity_;
    const auto historyRecords = logger.readRecent(200);
    for (const auto& hr : historyRecords) {
        if (hr.timestamp.size() >= 10) {
            combinedDays[hr.timestamp.substr(0, 10)] += 1;
        }
    }

    for (int offset = -27; offset <= 0; ++offset) {
        const std::string d = addDaysToToday(offset);
        const std::size_t cnt = combinedDays.count(d) ? combinedDays.at(d) : 0;
        snap.dailyHeatmap.push_back(DayActivity{d, cnt});
        if (offset >= -6) {
            snap.weeklyActivity.push_back(DayActivity{d, cnt});
        }
    }

    for (const auto& [topic, cnt] : topicCounts) {
        snap.mostPracticedTopics.push_back(TopicPracticeCount{topic, cnt});
    }
    std::sort(snap.mostPracticedTopics.begin(), snap.mostPracticedTopics.end(),
              [](const TopicPracticeCount& a, const TopicPracticeCount& b) {
                  return a.count > b.count;
              });
    if (snap.mostPracticedTopics.size() > 6) {
        snap.mostPracticedTopics.resize(6);
    }

    return snap;
}

} // namespace dsa
