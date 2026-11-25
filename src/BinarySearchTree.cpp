#include "dsa/BinarySearchTree.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace dsa {

int BinarySearchTree::height() const noexcept {
    return computeHeight(root_.get());
}

int BinarySearchTree::computeHeight(const Node* node) noexcept {
    if (!node) return 0;
    return 1 + std::max(computeHeight(node->left.get()), computeHeight(node->right.get()));
}

const BinarySearchTree::Node* BinarySearchTree::findMin(const Node* node) noexcept {
    const Node* curr = node;
    while (curr && curr->left) {
        curr = curr->left.get();
    }
    return curr;
}

void BinarySearchTree::collectInorder(const Node* node, std::vector<int>& out, std::vector<TreeStep>* steps) {
    if (!node) return;
    collectInorder(node->left.get(), out, steps);
    out.push_back(node->value);
    if (steps) {
        steps->push_back(TreeStep{node->value, "traverse", "Visited node " + std::to_string(node->value) + " (Inorder)."});
    }
    collectInorder(node->right.get(), out, steps);
}

void BinarySearchTree::collectPreorder(const Node* node, std::vector<int>& out, std::vector<TreeStep>* steps) {
    if (!node) return;
    out.push_back(node->value);
    if (steps) {
        steps->push_back(TreeStep{node->value, "traverse", "Visited node " + std::to_string(node->value) + " (Preorder)."});
    }
    collectPreorder(node->left.get(), out, steps);
    collectPreorder(node->right.get(), out, steps);
}

void BinarySearchTree::collectPostorder(const Node* node, std::vector<int>& out, std::vector<TreeStep>* steps) {
    if (!node) return;
    collectPostorder(node->left.get(), out, steps);
    collectPostorder(node->right.get(), out, steps);
    out.push_back(node->value);
    if (steps) {
        steps->push_back(TreeStep{node->value, "traverse", "Visited node " + std::to_string(node->value) + " (Postorder)."});
    }
}

void BinarySearchTree::collectSnapshots(const Node* node, int depth, int& inorderIdx,
                                        std::vector<TreeNodeSnapshot>& out) {
    if (!node) return;
    collectSnapshots(node->left.get(), depth + 1, inorderIdx, out);

    TreeNodeSnapshot snap;
    snap.value = node->value;
    snap.depth = depth;
    snap.orderIndex = inorderIdx++;
    snap.hasLeft = (node->left != nullptr);
    snap.leftValue = node->left ? node->left->value : 0;
    snap.hasRight = (node->right != nullptr);
    snap.rightValue = node->right ? node->right->value : 0;
    out.push_back(snap);

    collectSnapshots(node->right.get(), depth + 1, inorderIdx, out);
}

std::vector<TreeNodeSnapshot> BinarySearchTree::buildSnapshotNodes() const {
    std::vector<TreeNodeSnapshot> out;
    int idx = 0;
    collectSnapshots(root_.get(), 0, idx, out);
    return out;
}

BstOperationResult BinarySearchTree::insert(int value) {
    if (value < -9999 || value > 9999) {
        throw ValidationException("BST node value must be between -9999 and 9999.");
    }
    if (nodeCount_ >= 31) {
        throw OverflowException("BST capacity limit (31 nodes) reached for clear visualization.");
    }

    std::vector<TreeStep> steps;
    if (!root_) {
        root_ = std::make_unique<Node>(value);
        ++nodeCount_;
        steps.push_back(TreeStep{value, "insert", "Inserted " + std::to_string(value) + " as root node."});
        BstOperationResult res;
        res.operation = "insert";
        res.message = "Inserted " + std::to_string(value) + " as BST root.";
        res.steps = std::move(steps);
        res.nodes = buildSnapshotNodes();
        res.nodeCount = nodeCount_;
        res.height = height();
        return res;
    }

    Node* curr = root_.get();
    while (curr) {
        steps.push_back(TreeStep{curr->value, "visit", "Comparing " + std::to_string(value) + " with node " + std::to_string(curr->value) + "."});
        if (value == curr->value) {
            throw ValidationException("Duplicate value " + std::to_string(value) + " already exists in the BST.");
        }
        if (value < curr->value) {
            steps.push_back(TreeStep{curr->value, "go_left", std::to_string(value) + " < " + std::to_string(curr->value) + ": moving left."});
            if (!curr->left) {
                curr->left = std::make_unique<Node>(value);
                ++nodeCount_;
                steps.push_back(TreeStep{value, "insert", "Inserted " + std::to_string(value) + " as left child of " + std::to_string(curr->value) + "."});
                break;
            }
            curr = curr->left.get();
        } else {
            steps.push_back(TreeStep{curr->value, "go_right", std::to_string(value) + " > " + std::to_string(curr->value) + ": moving right."});
            if (!curr->right) {
                curr->right = std::make_unique<Node>(value);
                ++nodeCount_;
                steps.push_back(TreeStep{value, "insert", "Inserted " + std::to_string(value) + " as right child of " + std::to_string(curr->value) + "."});
                break;
            }
            curr = curr->right.get();
        }
    }

    BstOperationResult res;
    res.operation = "insert";
    res.message = "Inserted " + std::to_string(value) + " into BST.";
    res.steps = std::move(steps);
    res.nodes = buildSnapshotNodes();
    res.nodeCount = nodeCount_;
    res.height = height();
    return res;
}

BstOperationResult BinarySearchTree::search(int value) const {
    if (!root_) {
        throw UnderflowException("BST is empty. Insert nodes before searching.");
    }

    std::vector<TreeStep> steps;
    const Node* curr = root_.get();
    bool found = false;

    while (curr) {
        steps.push_back(TreeStep{curr->value, "visit", "Visiting node " + std::to_string(curr->value) + "."});
        if (value == curr->value) {
            found = true;
            steps.push_back(TreeStep{curr->value, "found", "Found target " + std::to_string(value) + " in BST."});
            break;
        }
        if (value < curr->value) {
            steps.push_back(TreeStep{curr->value, "go_left", std::to_string(value) + " < " + std::to_string(curr->value) + ": going left."});
            curr = curr->left.get();
        } else {
            steps.push_back(TreeStep{curr->value, "go_right", std::to_string(value) + " > " + std::to_string(curr->value) + ": going right."});
            curr = curr->right.get();
        }
    }

    if (!found) {
        steps.push_back(TreeStep{value, "not_found", "Reached null link. Value " + std::to_string(value) + " is not in the BST."});
    }

    BstOperationResult res;
    res.success = found;
    res.operation = "search";
    res.message = found ? ("Found node " + std::to_string(value) + " in BST.")
                        : ("Value " + std::to_string(value) + " was not found in BST.");
    res.steps = std::move(steps);
    res.nodes = buildSnapshotNodes();
    res.nodeCount = nodeCount_;
    res.height = height();
    return res;
}

std::unique_ptr<BinarySearchTree::Node> BinarySearchTree::removeNode(
    std::unique_ptr<Node> node, int value, std::vector<TreeStep>& steps, bool& removed) {
    if (!node) {
        return nullptr;
    }

    steps.push_back(TreeStep{node->value, "visit", "Checking node " + std::to_string(node->value) + "."});

    if (value < node->value) {
        steps.push_back(TreeStep{node->value, "go_left", std::to_string(value) + " < " + std::to_string(node->value) + ": searching left subtree."});
        node->left = removeNode(std::move(node->left), value, steps, removed);
        return node;
    }
    if (value > node->value) {
        steps.push_back(TreeStep{node->value, "go_right", std::to_string(value) + " > " + std::to_string(node->value) + ": searching right subtree."});
        node->right = removeNode(std::move(node->right), value, steps, removed);
        return node;
    }

    removed = true;
    steps.push_back(TreeStep{node->value, "delete", "Deleting node " + std::to_string(node->value) + "."});

    if (!node->left && !node->right) {
        return nullptr;
    }
    if (!node->left) {
        return std::move(node->right);
    }
    if (!node->right) {
        return std::move(node->left);
    }

    const Node* successor = findMin(node->right.get());
    const int succVal = successor->value;
    steps.push_back(TreeStep{succVal, "visit", "Replacing " + std::to_string(node->value) + " with inorder successor " + std::to_string(succVal) + "."});
    node->value = succVal;
    bool dummy = false;
    node->right = removeNode(std::move(node->right), succVal, steps, dummy);
    return node;
}

BstOperationResult BinarySearchTree::remove(int value) {
    if (!root_) {
        throw UnderflowException("BST Underflow: cannot delete from an empty tree.");
    }

    std::vector<TreeStep> steps;
    bool removed = false;
    root_ = removeNode(std::move(root_), value, steps, removed);
    if (!removed) {
        throw ValidationException("Value " + std::to_string(value) + " does not exist in the BST.");
    }
    --nodeCount_;

    BstOperationResult res;
    res.operation = "delete";
    res.message = "Deleted node " + std::to_string(value) + " from BST.";
    res.steps = std::move(steps);
    res.nodes = buildSnapshotNodes();
    res.nodeCount = nodeCount_;
    res.height = height();
    return res;
}

BstOperationResult BinarySearchTree::traverse(const std::string& order) const {
    if (!root_) {
        throw UnderflowException("BST is empty. Insert nodes before running traversals.");
    }

    std::vector<int> out;
    std::vector<TreeStep> steps;
    if (order == "inorder") {
        collectInorder(root_.get(), out, &steps);
    } else if (order == "preorder") {
        collectPreorder(root_.get(), out, &steps);
    } else if (order == "postorder") {
        collectPostorder(root_.get(), out, &steps);
    } else {
        throw ValidationException("Unknown traversal order: '" + order + "'.");
    }

    BstOperationResult res;
    res.operation = order;
    res.message = order + " traversal complete (" + std::to_string(out.size()) + " nodes).";
    res.traversalOrder = std::move(out);
    res.steps = std::move(steps);
    res.nodes = buildSnapshotNodes();
    res.nodeCount = nodeCount_;
    res.height = height();
    return res;
}

BstOperationResult BinarySearchTree::loadSample() {
    clear();
    const std::vector<int> sample{50, 30, 70, 20, 40, 60, 80};
    for (int v : sample) {
        insert(v);
    }
    BstOperationResult res = snapshot();
    res.operation = "sample";
    res.message = "Loaded balanced 7-node sample BST (50, 30, 70, 20, 40, 60, 80).";
    return res;
}

BstOperationResult BinarySearchTree::clear() {
    root_.reset();
    nodeCount_ = 0;
    BstOperationResult res;
    res.operation = "clear";
    res.message = "Cleared all nodes from BST.";
    return res;
}

BstOperationResult BinarySearchTree::snapshot() const {
    BstOperationResult res;
    res.operation = "snapshot";
    res.message = "BST state loaded.";
    res.nodes = buildSnapshotNodes();
    res.nodeCount = nodeCount_;
    res.height = height();
    return res;
}

} // namespace dsa
