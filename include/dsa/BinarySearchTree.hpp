#pragma once

#include "dsa/Types.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace dsa {

class BinarySearchTree {
public:
    BinarySearchTree() = default;
    ~BinarySearchTree() = default;

    BinarySearchTree(const BinarySearchTree&) = delete;
    BinarySearchTree& operator=(const BinarySearchTree&) = delete;

    BstOperationResult insert(int value);
    BstOperationResult search(int value) const;
    BstOperationResult remove(int value);
    BstOperationResult traverse(const std::string& order) const;
    BstOperationResult loadSample();
    BstOperationResult clear();
    BstOperationResult snapshot() const;

    std::size_t size() const noexcept { return nodeCount_; }
    bool isEmpty() const noexcept { return root_ == nullptr; }
    int height() const noexcept;

private:
    struct Node {
        int value;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
        explicit Node(int v) : value(v) {}
    };

    std::unique_ptr<Node> root_;
    std::size_t nodeCount_ = 0;

    static int computeHeight(const Node* node) noexcept;
    static std::unique_ptr<Node> removeNode(std::unique_ptr<Node> node, int value,
                                            std::vector<TreeStep>& steps, bool& removed);
    static const Node* findMin(const Node* node) noexcept;
    static void collectInorder(const Node* node, std::vector<int>& out, std::vector<TreeStep>* steps);
    static void collectPreorder(const Node* node, std::vector<int>& out, std::vector<TreeStep>* steps);
    static void collectPostorder(const Node* node, std::vector<int>& out, std::vector<TreeStep>* steps);
    static void collectSnapshots(const Node* node, int depth, int& inorderIdx,
                                 std::vector<TreeNodeSnapshot>& out);
    std::vector<TreeNodeSnapshot> buildSnapshotNodes() const;
};

} // namespace dsa
