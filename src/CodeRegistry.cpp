#include "dsa/CodeRegistry.hpp"

namespace dsa {

std::vector<AlgorithmCodeEntry> getAllAlgorithmCodeEntries() {
    return {
        {
            "bubble",
            "Bubble Sort",
            "Sorting",
            "O(n)",
            "O(n^2)",
            "O(n^2)",
            "O(1)",
            R"(void bubbleSort(std::vector<int>& arr) {
    const std::size_t n = arr.size();
    for (std::size_t i = 0; i < n - 1; ++i) {
        bool swapped = false;
        for (std::size_t j = 0; j < n - i - 1; ++j) {
            if (arr[j] > arr[j + 1]) {
                std::swap(arr[j], arr[j + 1]);
                swapped = true;
            }
        }
        if (!swapped) break; // Already sorted
    }
})",
            R"(procedure BubbleSort(A):
    n <- length(A)
    for i <- 0 to n - 2:
        swapped <- false
        for j <- 0 to n - i - 2:
            if A[j] > A[j + 1]:
                swap(A[j], A[j + 1])
                swapped <- true
        if not swapped:
            break)"
        },
        {
            "selection",
            "Selection Sort",
            "Sorting",
            "O(n^2)",
            "O(n^2)",
            "O(n^2)",
            "O(1)",
            R"(void selectionSort(std::vector<int>& arr) {
    const std::size_t n = arr.size();
    for (std::size_t i = 0; i < n - 1; ++i) {
        std::size_t minIdx = i;
        for (std::size_t j = i + 1; j < n; ++j) {
            if (arr[j] < arr[minIdx]) {
                minIdx = j;
            }
        }
        if (minIdx != i) {
            std::swap(arr[i], arr[minIdx]);
        }
    }
})",
            R"(procedure SelectionSort(A):
    n <- length(A)
    for i <- 0 to n - 2:
        minIdx <- i
        for j <- i + 1 to n - 1:
            if A[j] < A[minIdx]:
                minIdx <- j
        if minIdx != i:
            swap(A[i], A[minIdx]))"
        },
        {
            "insertion",
            "Insertion Sort",
            "Sorting",
            "O(n)",
            "O(n^2)",
            "O(n^2)",
            "O(1)",
            R"(void insertionSort(std::vector<int>& arr) {
    const std::size_t n = arr.size();
    for (std::size_t i = 1; i < n; ++i) {
        std::size_t j = i;
        while (j > 0 && arr[j - 1] > arr[j]) {
            std::swap(arr[j - 1], arr[j]);
            --j;
        }
    }
})",
            R"(procedure InsertionSort(A):
    n <- length(A)
    for i <- 1 to n - 1:
        j <- i
        while j > 0 and A[j - 1] > A[j]:
            swap(A[j - 1], A[j])
            j <- j - 1)"
        },
        {
            "linear",
            "Linear Search",
            "Searching",
            "O(1)",
            "O(n)",
            "O(n)",
            "O(1)",
            R"(int linearSearch(const std::vector<int>& arr, int target) {
    for (std::size_t i = 0; i < arr.size(); ++i) {
        if (arr[i] == target) {
            return static_cast<int>(i);
        }
    }
    return -1;
})",
            R"(procedure LinearSearch(A, target):
    for i <- 0 to length(A) - 1:
        if A[i] == target:
            return i
    return -1)"
        },
        {
            "binary",
            "Binary Search",
            "Searching",
            "O(1)",
            "O(log n)",
            "O(log n)",
            "O(1)",
            R"(int binarySearch(const std::vector<int>& arr, int target) {
    int low = 0;
    int high = static_cast<int>(arr.size()) - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (arr[mid] == target) return mid;
        if (arr[mid] < target) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
})",
            R"(procedure BinarySearch(A, target):
    low <- 0
    high <- length(A) - 1
    while low <= high:
        mid <- low + (high - low) / 2
        if A[mid] == target: return mid
        if A[mid] < target: low <- mid + 1
        else: high <- mid - 1
    return -1)"
        },
        {
            "bst",
            "Binary Search Tree (Insert / Search / Delete)",
            "Trees",
            "O(log n)",
            "O(log n)",
            "O(n)",
            "O(n)",
            R"(Node* insertBST(Node* root, int key) {
    if (!root) return new Node(key);
    if (key < root->value) root->left = insertBST(root->left, key);
    else if (key > root->value) root->right = insertBST(root->right, key);
    return root;
}

void inorder(const Node* root) {
    if (!root) return;
    inorder(root->left);
    process(root->value);
    inorder(root->right);
})",
            R"(procedure InsertBST(node, key):
    if node is null:
        return new Node(key)
    if key < node.value:
        node.left <- InsertBST(node.left, key)
    else if key > node.value:
        node.right <- InsertBST(node.right, key)
    return node

procedure Inorder(node):
    if node is null: return
    Inorder(node.left)
    visit(node.value)
    Inorder(node.right))"
        },
        {
            "bfs",
            "Breadth-First Search (BFS)",
            "Graphs",
            "O(V + E)",
            "O(V + E)",
            "O(V + E)",
            "O(V)",
            R"(std::vector<std::string> bfs(const AdjList& adj, const std::string& start) {
    std::set<std::string> seen{start};
    std::queue<std::string> q;
    std::vector<std::string> order;
    q.push(start);

    while (!q.empty()) {
        auto curr = q.front();
        q.pop();
        order.push_back(curr);
        for (const auto& nbr : adj.at(curr)) {
            if (seen.insert(nbr).second) {
                q.push(nbr);
            }
        }
    }
    return order;
})",
            R"(procedure BFS(G, start):
    visited <- {start}
    queue <- [start]
    while queue is not empty:
        curr <- dequeue(queue)
        output(curr)
        for each neighbor nbr of curr in G:
            if nbr not in visited:
                add nbr to visited
                enqueue(queue, nbr))"
        },
        {
            "dfs",
            "Depth-First Search (DFS)",
            "Graphs",
            "O(V + E)",
            "O(V + E)",
            "O(V + E)",
            "O(V)",
            R"(std::vector<std::string> dfs(const AdjList& adj, const std::string& start) {
    std::set<std::string> visited;
    std::stack<std::string> st;
    std::vector<std::string> order;
    st.push(start);

    while (!st.empty()) {
        auto curr = st.top();
        st.pop();
        if (!visited.insert(curr).second) continue;
        order.push_back(curr);
        const auto& nbrs = adj.at(curr);
        for (auto it = nbrs.rbegin(); it != nbrs.rend(); ++it) {
            if (!visited.count(*it)) st.push(*it);
        }
    }
    return order;
})",
            R"(procedure DFS(G, start):
    visited <- empty set
    stack <- [start]
    while stack is not empty:
        curr <- pop(stack)
        if curr not in visited:
            add curr to visited
            output(curr)
            for each neighbor nbr of curr in reverse order:
                if nbr not in visited:
                    push(stack, nbr))"
        }
    };
}

} // namespace dsa
