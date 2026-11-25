#include "dsa/Algorithms.hpp"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <random>
#include <sstream>

namespace dsa {

namespace utils {

std::vector<int> generateRandomArray(std::size_t size) {
    return generateRandomArray(size, 10, 99);
}

std::vector<int> generateRandomArray(std::size_t size, int minVal, int maxVal) {
    if (size < 2 || size > 60) {
        throw ValidationException("Array size must be between 2 and 60.");
    }
    if (minVal >= maxVal) {
        throw ValidationException("Minimum value must be strictly less than maximum value.");
    }
    if (minVal < -9999 || maxVal > 9999) {
        throw ValidationException("Array values must be within [-9999, 9999].");
    }

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(minVal, maxVal);

    std::vector<int> result(size);
    for (std::size_t i = 0; i < size; ++i) {
        result[i] = dist(rng);
    }
    return result;
}

void validateArrayInput(const std::vector<int>& arr, std::size_t minSize, std::size_t maxSize) {
    if (arr.size() < minSize || arr.size() > maxSize) {
        std::ostringstream oss;
        oss << "Array must contain between " << minSize << " and " << maxSize << " elements.";
        throw ValidationException(oss.str());
    }
    for (int val : arr) {
        if (val < -9999 || val > 9999) {
            throw ValidationException("Array elements must be integers between -9999 and 9999.");
        }
    }
}

std::vector<int> generatePatternArray(std::size_t size, const std::string& pattern) {
    if (size < 2 || size > 2000) {
        throw ValidationException("Benchmark array size must be between 2 and 2000.");
    }
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, 9999);

    std::vector<int> arr(size);
    if (pattern == "random") {
        for (std::size_t i = 0; i < size; ++i) arr[i] = dist(rng);
    } else if (pattern == "reversed") {
        for (std::size_t i = 0; i < size; ++i) arr[i] = static_cast<int>(size - i);
    } else if (pattern == "sorted") {
        for (std::size_t i = 0; i < size; ++i) arr[i] = static_cast<int>(i + 1);
    } else if (pattern == "nearly_sorted") {
        for (std::size_t i = 0; i < size; ++i) arr[i] = static_cast<int>(i + 1);
        std::size_t swaps = std::max<std::size_t>(1, size / 10);
        std::uniform_int_distribution<std::size_t> idxDist(0, size - 1);
        for (std::size_t s = 0; s < swaps; ++s) {
            swapValues(arr[idxDist(rng)], arr[idxDist(rng)]);
        }
    } else {
        throw ValidationException("Unknown array pattern: '" + pattern + "'.");
    }
    return arr;
}

} // namespace utils

// ============================================================================
// Algorithm Base
// ============================================================================

Algorithm::Algorithm(AlgorithmInfo info) : info_(std::move(info)) {}

const AlgorithmInfo& Algorithm::getInfo() const noexcept {
    return info_;
}

namespace {

std::vector<int> makeRangeIndices(std::size_t count) {
    std::vector<int> indices(count);
    std::iota(indices.begin(), indices.end(), 0);
    return indices;
}

} // namespace

// ============================================================================
// Sorting Algorithms
// ============================================================================

BubbleSorter::BubbleSorter()
    : Sorter(AlgorithmInfo{
          "bubble",
          "Bubble Sort",
          "Sorting",
          "Repeatedly compares adjacent elements and swaps them if they are in the wrong order, bubbling larger values to the end on each pass.",
          "O(n)",
          "O(n^2)",
          "O(n^2)",
          "O(1)",
          "Stable, in-place comparison sort. Uses an early-exit flag when a pass performs zero swaps."}) {}

SortResult BubbleSorter::sort(const std::vector<int>& input) const {
    utils::validateArrayInput(input, 2, 60);

    SortResult result;
    result.info = info_;
    result.initialArray = input;

    std::vector<int> arr = input;
    const std::size_t n = arr.size();
    std::vector<int> sortedIndices;
    Metrics metrics;

    result.steps.push_back(SortStep{
        arr, -1, -1, sortedIndices, "init", "Initial array loaded into Bubble Sort.", 0, 0});

    for (std::size_t i = 0; i < n - 1; ++i) {
        bool swappedInPass = false;
        for (std::size_t j = 0; j < n - i - 1; ++j) {
            ++metrics.comparisons;
            result.steps.push_back(SortStep{
                arr,
                static_cast<int>(j),
                static_cast<int>(j + 1),
                sortedIndices,
                "compare",
                "Comparing index " + std::to_string(j) + " (" + std::to_string(arr[j]) +
                    ") and index " + std::to_string(j + 1) + " (" + std::to_string(arr[j + 1]) + ").",
                metrics.comparisons,
                metrics.swaps});

            if (arr[j] > arr[j + 1]) {
                utils::swapValues(arr[j], arr[j + 1]);
                ++metrics.swaps;
                swappedInPass = true;
                result.steps.push_back(SortStep{
                    arr,
                    static_cast<int>(j),
                    static_cast<int>(j + 1),
                    sortedIndices,
                    "swap",
                    "Swapped " + std::to_string(arr[j + 1]) + " and " + std::to_string(arr[j]) + ".",
                    metrics.comparisons,
                    metrics.swaps});
            }
        }
        sortedIndices.push_back(static_cast<int>(n - 1 - i));
        if (!swappedInPass) {
            break;
        }
    }

    sortedIndices = makeRangeIndices(n);
    result.steps.push_back(SortStep{
        arr,
        -1,
        -1,
        sortedIndices,
        "done",
        "Bubble Sort complete.",
        metrics.comparisons,
        metrics.swaps});

    result.sortedArray = std::move(arr);
    result.metrics = metrics;
    return result;
}

SelectionSorter::SelectionSorter()
    : Sorter(AlgorithmInfo{
          "selection",
          "Selection Sort",
          "Sorting",
          "Divides the array into sorted and unsorted regions, repeatedly finding the minimum element from the unsorted region and placing it at the front.",
          "O(n^2)",
          "O(n^2)",
          "O(n^2)",
          "O(1)",
          "In-place sort that minimizes the number of swaps (at most n - 1 swaps)."}) {}

SortResult SelectionSorter::sort(const std::vector<int>& input) const {
    utils::validateArrayInput(input, 2, 60);

    SortResult result;
    result.info = info_;
    result.initialArray = input;

    std::vector<int> arr = input;
    const std::size_t n = arr.size();
    std::vector<int> sortedIndices;
    Metrics metrics;

    result.steps.push_back(SortStep{
        arr, -1, -1, sortedIndices, "init", "Initial array loaded into Selection Sort.", 0, 0});

    for (std::size_t i = 0; i < n - 1; ++i) {
        std::size_t minIdx = i;
        for (std::size_t j = i + 1; j < n; ++j) {
            ++metrics.comparisons;
            result.steps.push_back(SortStep{
                arr,
                static_cast<int>(minIdx),
                static_cast<int>(j),
                sortedIndices,
                "compare",
                "Comparing current minimum " + std::to_string(arr[minIdx]) + " (idx " +
                    std::to_string(minIdx) + ") with " + std::to_string(arr[j]) + " (idx " +
                    std::to_string(j) + ").",
                metrics.comparisons,
                metrics.swaps});

            if (arr[j] < arr[minIdx]) {
                minIdx = j;
            }
        }

        if (minIdx != i) {
            utils::swapValues(arr[i], arr[minIdx]);
            ++metrics.swaps;
            result.steps.push_back(SortStep{
                arr,
                static_cast<int>(i),
                static_cast<int>(minIdx),
                sortedIndices,
                "swap",
                "Placed minimum value " + std::to_string(arr[i]) + " at index " + std::to_string(i) + ".",
                metrics.comparisons,
                metrics.swaps});
        }
        sortedIndices.push_back(static_cast<int>(i));
    }

    sortedIndices = makeRangeIndices(n);
    result.steps.push_back(SortStep{
        arr,
        -1,
        -1,
        sortedIndices,
        "done",
        "Selection Sort complete.",
        metrics.comparisons,
        metrics.swaps});

    result.sortedArray = std::move(arr);
    result.metrics = metrics;
    return result;
}

InsertionSorter::InsertionSorter()
    : Sorter(AlgorithmInfo{
          "insertion",
          "Insertion Sort",
          "Sorting",
          "Builds the sorted array one item at a time by taking each element and inserting it into its proper position among previously sorted elements.",
          "O(n)",
          "O(n^2)",
          "O(n^2)",
          "O(1)",
          "Stable, adaptive, in-place sort; very efficient on small or nearly sorted datasets."}) {}

SortResult InsertionSorter::sort(const std::vector<int>& input) const {
    utils::validateArrayInput(input, 2, 60);

    SortResult result;
    result.info = info_;
    result.initialArray = input;

    std::vector<int> arr = input;
    const std::size_t n = arr.size();
    std::vector<int> sortedIndices{0};
    Metrics metrics;

    result.steps.push_back(SortStep{
        arr, -1, -1, sortedIndices, "init", "Initial array loaded into Insertion Sort.", 0, 0});

    for (std::size_t i = 1; i < n; ++i) {
        std::size_t j = i;
        while (j > 0) {
            ++metrics.comparisons;
            result.steps.push_back(SortStep{
                arr,
                static_cast<int>(j - 1),
                static_cast<int>(j),
                sortedIndices,
                "compare",
                "Comparing " + std::to_string(arr[j - 1]) + " (idx " + std::to_string(j - 1) +
                    ") with key " + std::to_string(arr[j]) + " (idx " + std::to_string(j) + ").",
                metrics.comparisons,
                metrics.swaps});

            if (arr[j - 1] > arr[j]) {
                utils::swapValues(arr[j - 1], arr[j]);
                ++metrics.swaps;
                result.steps.push_back(SortStep{
                    arr,
                    static_cast<int>(j - 1),
                    static_cast<int>(j),
                    sortedIndices,
                    "swap",
                    "Moved " + std::to_string(arr[j - 1]) + " left to index " + std::to_string(j - 1) + ".",
                    metrics.comparisons,
                    metrics.swaps});
                --j;
            } else {
                break;
            }
        }
        sortedIndices.push_back(static_cast<int>(i));
    }

    sortedIndices = makeRangeIndices(n);
    result.steps.push_back(SortStep{
        arr,
        -1,
        -1,
        sortedIndices,
        "done",
        "Insertion Sort complete.",
        metrics.comparisons,
        metrics.swaps});

    result.sortedArray = std::move(arr);
    result.metrics = metrics;
    return result;
}

// ============================================================================
// Searching Algorithms
// ============================================================================

LinearSearcher::LinearSearcher()
    : Searcher(AlgorithmInfo{
          "linear",
          "Linear Search",
          "Searching",
          "Sequentially checks each element from index 0 until the target value is matched or the end of the array is reached.",
          "O(1)",
          "O(n)",
          "O(n)",
          "O(1)",
          "Works on both unsorted and sorted arrays without preprocessing."}) {}

SearchResult LinearSearcher::search(const std::vector<int>& input, int target) const {
    utils::validateArrayInput(input, 1, 60);

    SearchResult result;
    result.info = info_;
    result.arrayUsed = input;
    result.target = target;

    for (std::size_t i = 0; i < input.size(); ++i) {
        ++result.comparisons;
        result.steps.push_back(SearchStep{
            static_cast<int>(i),
            0,
            static_cast<int>(input.size()) - 1,
            static_cast<int>(i),
            input[i],
            "inspect",
            "Inspecting index " + std::to_string(i) + " (value " + std::to_string(input[i]) + ").",
            result.comparisons});

        if (input[i] == target) {
            result.found = true;
            result.foundIndex = static_cast<int>(i);
            result.steps.push_back(SearchStep{
                static_cast<int>(i),
                0,
                static_cast<int>(input.size()) - 1,
                static_cast<int>(i),
                input[i],
                "found",
                "Target " + std::to_string(target) + " found at index " + std::to_string(i) + ".",
                result.comparisons});
            return result;
        }
    }

    result.steps.push_back(SearchStep{
        -1,
        0,
        static_cast<int>(input.size()) - 1,
        -1,
        0,
        "not_found",
        "Target " + std::to_string(target) + " was not found in the array.",
        result.comparisons});
    return result;
}

BinarySearcher::BinarySearcher()
    : Searcher(AlgorithmInfo{
          "binary",
          "Binary Search",
          "Searching",
          "Repeatedly halves a sorted search interval by comparing the target with the middle element and discarding the half that cannot contain the target.",
          "O(1)",
          "O(log n)",
          "O(log n)",
          "O(1)",
          "Requires the input array to be sorted in ascending order."}) {}

SearchResult BinarySearcher::search(const std::vector<int>& input, int target) const {
    utils::validateArrayInput(input, 1, 60);
    if (!utils::isSorted(input)) {
        throw ValidationException("Binary Search requires a sorted array in ascending order.");
    }

    SearchResult result;
    result.info = info_;
    result.arrayUsed = input;
    result.target = target;

    int low = 0;
    int high = static_cast<int>(input.size()) - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        ++result.comparisons;

        result.steps.push_back(SearchStep{
            mid,
            low,
            high,
            mid,
            input[static_cast<std::size_t>(mid)],
            "inspect",
            "Checking mid=" + std::to_string(mid) + " (value " +
                std::to_string(input[static_cast<std::size_t>(mid)]) + ") in range [" +
                std::to_string(low) + ", " + std::to_string(high) + "].",
            result.comparisons});

        if (input[static_cast<std::size_t>(mid)] == target) {
            result.found = true;
            result.foundIndex = mid;
            result.steps.push_back(SearchStep{
                mid,
                low,
                high,
                mid,
                input[static_cast<std::size_t>(mid)],
                "found",
                "Target " + std::to_string(target) + " found at index " + std::to_string(mid) + ".",
                result.comparisons});
            return result;
        }

        if (input[static_cast<std::size_t>(mid)] < target) {
            low = mid + 1;
            result.steps.push_back(SearchStep{
                mid,
                low,
                high,
                mid,
                input[static_cast<std::size_t>(mid)],
                "discard_left",
                std::to_string(input[static_cast<std::size_t>(mid)]) + " < " + std::to_string(target) +
                    ": discarding left half; new range [" + std::to_string(low) + ", " +
                    std::to_string(high) + "].",
                result.comparisons});
        } else {
            high = mid - 1;
            result.steps.push_back(SearchStep{
                mid,
                low,
                high,
                mid,
                input[static_cast<std::size_t>(mid)],
                "discard_right",
                std::to_string(input[static_cast<std::size_t>(mid)]) + " > " + std::to_string(target) +
                    ": discarding right half; new range [" + std::to_string(low) + ", " +
                    std::to_string(high) + "].",
                result.comparisons});
        }
    }

    result.steps.push_back(SearchStep{
        -1,
        low,
        high,
        -1,
        0,
        "not_found",
        "Search interval exhausted. Target " + std::to_string(target) + " not found.",
        result.comparisons});
    return result;
}

// ============================================================================
// Factory & Info Registry
// ============================================================================

std::unique_ptr<Sorter> createSorter(const std::string& id) {
    if (id == "bubble") {
        return std::make_unique<BubbleSorter>();
    }
    if (id == "selection") {
        return std::make_unique<SelectionSorter>();
    }
    if (id == "insertion") {
        return std::make_unique<InsertionSorter>();
    }
    throw ValidationException("Unknown sorting algorithm: '" + id + "'.");
}

std::unique_ptr<Searcher> createSearcher(const std::string& id) {
    if (id == "linear") {
        return std::make_unique<LinearSearcher>();
    }
    if (id == "binary") {
        return std::make_unique<BinarySearcher>();
    }
    throw ValidationException("Unknown searching algorithm: '" + id + "'.");
}

std::vector<AlgorithmInfo> getAllAlgorithmInfo() {
    std::vector<AlgorithmInfo> infos;
    infos.push_back(BubbleSorter().getInfo());
    infos.push_back(SelectionSorter().getInfo());
    infos.push_back(InsertionSorter().getInfo());
    infos.push_back(LinearSearcher().getInfo());
    infos.push_back(BinarySearcher().getInfo());
    infos.push_back(AlgorithmInfo{
        "stack",
        "Stack (LIFO)",
        "Data Structure",
        "A Last-In, First-Out linear container where Push, Pop, and Peek operate strictly on the top element.",
        "O(1) push/pop/peek",
        "O(1) push/pop/peek",
        "O(1) push/pop/peek",
        "O(n)",
        "Backed by std::stack<int> with bounded capacity checks for overflow and underflow."});
    infos.push_back(AlgorithmInfo{
        "queue",
        "Queue (FIFO)",
        "Data Structure",
        "A First-In, First-Out linear container where Enqueue inserts at the rear and Dequeue/Front operate at the front.",
        "O(1) enqueue/dequeue",
        "O(1) enqueue/dequeue",
        "O(1) enqueue/dequeue",
        "O(n)",
        "Backed by std::queue<int> with bounded capacity checks for overflow and underflow."});
    infos.push_back(AlgorithmInfo{
        "bst",
        "Binary Search Tree",
        "Trees",
        "Hierarchical node structure where left subtree keys are smaller and right subtree keys are larger than each node.",
        "O(log n)",
        "O(log n)",
        "O(n)",
        "O(n)",
        "Supports insert, search, 3-case delete, and inorder/preorder/postorder traversals."});
    infos.push_back(AlgorithmInfo{
        "bfs",
        "Breadth-First Search (BFS)",
        "Graphs",
        "Explores a graph level by level from a start vertex using a FIFO queue.",
        "O(V + E)",
        "O(V + E)",
        "O(V + E)",
        "O(V)",
        "Finds shortest paths in unweighted graphs by edge count."});
    infos.push_back(AlgorithmInfo{
        "dfs",
        "Depth-First Search (DFS)",
        "Graphs",
        "Explores as deep as possible along each branch before backtracking using a stack.",
        "O(V + E)",
        "O(V + E)",
        "O(V + E)",
        "O(V)",
        "Useful for cycle detection, connectivity, and topological ordering."});
    return infos;
}

std::vector<BenchmarkResult> compareSortingAlgorithms(const std::vector<int>& input) {
    utils::validateArrayInput(input, 2, 2000);

    auto runBench = [&](const AlgorithmInfo& info, auto sortFn) {
        std::vector<int> arr = input;
        Metrics metrics;
        const auto t0 = std::chrono::steady_clock::now();
        sortFn(arr, metrics);
        const auto t1 = std::chrono::steady_clock::now();
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        return BenchmarkResult{
            info,
            input.size(),
            metrics.comparisons,
            metrics.swaps,
            static_cast<double>(ns) / 1000.0,
            ns};
    };

    std::vector<BenchmarkResult> results;
    results.push_back(runBench(BubbleSorter().getInfo(), [](std::vector<int>& arr, Metrics& m) {
        const std::size_t n = arr.size();
        for (std::size_t i = 0; i < n - 1; ++i) {
            bool swapped = false;
            for (std::size_t j = 0; j < n - i - 1; ++j) {
                ++m.comparisons;
                if (arr[j] > arr[j + 1]) {
                    utils::swapValues(arr[j], arr[j + 1]);
                    ++m.swaps;
                    swapped = true;
                }
            }
            if (!swapped) break;
        }
    }));

    results.push_back(runBench(SelectionSorter().getInfo(), [](std::vector<int>& arr, Metrics& m) {
        const std::size_t n = arr.size();
        for (std::size_t i = 0; i < n - 1; ++i) {
            std::size_t minIdx = i;
            for (std::size_t j = i + 1; j < n; ++j) {
                ++m.comparisons;
                if (arr[j] < arr[minIdx]) {
                    minIdx = j;
                }
            }
            if (minIdx != i) {
                utils::swapValues(arr[i], arr[minIdx]);
                ++m.swaps;
            }
        }
    }));

    results.push_back(runBench(InsertionSorter().getInfo(), [](std::vector<int>& arr, Metrics& m) {
        const std::size_t n = arr.size();
        for (std::size_t i = 1; i < n; ++i) {
            std::size_t j = i;
            while (j > 0) {
                ++m.comparisons;
                if (arr[j - 1] > arr[j]) {
                    utils::swapValues(arr[j - 1], arr[j]);
                    ++m.swaps;
                    --j;
                } else {
                    break;
                }
            }
        }
    }));

    return results;
}

} // namespace dsa
