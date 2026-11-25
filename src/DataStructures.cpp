#include "dsa/DataStructures.hpp"
#include "dsa/Algorithms.hpp"

#include <algorithm>
#include <sstream>

namespace dsa {

// ============================================================================
// DataStructure Base
// ============================================================================

DataStructure::DataStructure(std::string name, std::size_t capacity)
    : name_(std::move(name)), capacity_(capacity) {
    validateCapacity(capacity_);
}

void DataStructure::validateCapacity(std::size_t capacity) {
    if (capacity < 2 || capacity > 16) {
        throw ValidationException("Capacity must be between 2 and 16.");
    }
}

void DataStructure::setCapacity(std::size_t newCapacity) {
    validateCapacity(newCapacity);
    if (newCapacity < size()) {
        throw ValidationException("New capacity cannot be smaller than current element count (" +
                                  std::to_string(size()) + ").");
    }
    capacity_ = newCapacity;
}

// ============================================================================
// BoundedStack
// ============================================================================

BoundedStack::BoundedStack(std::size_t capacity)
    : DataStructure("Stack", capacity) {}

void BoundedStack::push(int value) {
    if (value < -99999 || value > 99999) {
        throw ValidationException("Value must be an integer between -99999 and 99999.");
    }
    if (isFull()) {
        throw OverflowException("Stack Overflow: cannot push onto a full stack (capacity " +
                                std::to_string(capacity_) + ").");
    }
    stack_.push(value);
}

void BoundedStack::push(const std::vector<int>& values) {
    if (size() + values.size() > capacity_) {
        throw OverflowException("Stack Overflow: batch push exceeds capacity (" +
                                std::to_string(capacity_) + ").");
    }
    for (int v : values) {
        push(v);
    }
}

int BoundedStack::pop() {
    if (isEmpty()) {
        throw UnderflowException("Stack Underflow: cannot pop from an empty stack.");
    }
    int topVal = stack_.top();
    stack_.pop();
    return topVal;
}

int BoundedStack::peek() const {
    if (isEmpty()) {
        throw UnderflowException("Stack Underflow: cannot peek into an empty stack.");
    }
    return stack_.top();
}

BoundedStack& BoundedStack::operator+=(int value) {
    push(value);
    return *this;
}

std::size_t BoundedStack::size() const noexcept {
    return stack_.size();
}

bool BoundedStack::isEmpty() const noexcept {
    return stack_.empty();
}

bool BoundedStack::isFull() const noexcept {
    return stack_.size() >= capacity_;
}

void BoundedStack::clear() {
    while (!stack_.empty()) {
        stack_.pop();
    }
}

void BoundedStack::setCapacity(std::size_t newCapacity) {
    DataStructure::setCapacity(newCapacity);
}

std::vector<int> BoundedStack::snapshotBottomToTop() const {
    std::stack<int> copy = stack_;
    std::vector<int> result;
    result.reserve(copy.size());
    while (!copy.empty()) {
        result.push_back(copy.top());
        copy.pop();
    }
    std::reverse(result.begin(), result.end());
    return result;
}

DataStructureState BoundedStack::state() const {
    DataStructureState s;
    s.name = name_;
    s.elements = snapshotBottomToTop();
    s.size = size();
    s.capacity = capacity_;
    s.isEmpty = isEmpty();
    s.isFull = isFull();
    s.hasTopOrFront = !isEmpty();
    s.topOrFrontValue = s.hasTopOrFront ? stack_.top() : 0;
    return s;
}

// ============================================================================
// BoundedQueue
// ============================================================================

BoundedQueue::BoundedQueue(std::size_t capacity)
    : DataStructure("Queue", capacity) {}

void BoundedQueue::enqueue(int value) {
    if (value < -99999 || value > 99999) {
        throw ValidationException("Value must be an integer between -99999 and 99999.");
    }
    if (isFull()) {
        throw OverflowException("Queue Overflow: cannot enqueue into a full queue (capacity " +
                                std::to_string(capacity_) + ").");
    }
    queue_.push(value);
}

void BoundedQueue::enqueue(const std::vector<int>& values) {
    if (size() + values.size() > capacity_) {
        throw OverflowException("Queue Overflow: batch enqueue exceeds capacity (" +
                                std::to_string(capacity_) + ").");
    }
    for (int v : values) {
        enqueue(v);
    }
}

int BoundedQueue::dequeue() {
    if (isEmpty()) {
        throw UnderflowException("Queue Underflow: cannot dequeue from an empty queue.");
    }
    int frontVal = queue_.front();
    queue_.pop();
    return frontVal;
}

int BoundedQueue::front() const {
    if (isEmpty()) {
        throw UnderflowException("Queue Underflow: cannot read front of an empty queue.");
    }
    return queue_.front();
}

BoundedQueue& BoundedQueue::operator+=(int value) {
    enqueue(value);
    return *this;
}

std::size_t BoundedQueue::size() const noexcept {
    return queue_.size();
}

bool BoundedQueue::isEmpty() const noexcept {
    return queue_.empty();
}

bool BoundedQueue::isFull() const noexcept {
    return queue_.size() >= capacity_;
}

void BoundedQueue::clear() {
    while (!queue_.empty()) {
        queue_.pop();
    }
}

void BoundedQueue::setCapacity(std::size_t newCapacity) {
    DataStructure::setCapacity(newCapacity);
}

std::vector<int> BoundedQueue::snapshotFrontToRear() const {
    std::queue<int> copy = queue_;
    std::vector<int> result;
    result.reserve(copy.size());
    while (!copy.empty()) {
        result.push_back(copy.front());
        copy.pop();
    }
    return result;
}

DataStructureState BoundedQueue::state() const {
    DataStructureState s;
    s.name = name_;
    s.elements = snapshotFrontToRear();
    s.size = size();
    s.capacity = capacity_;
    s.isEmpty = isEmpty();
    s.isFull = isFull();
    s.hasTopOrFront = !isEmpty();
    s.topOrFrontValue = s.hasTopOrFront ? queue_.front() : 0;
    return s;
}

// ============================================================================
// BoundedLinkedList
// ============================================================================

BoundedLinkedList::BoundedLinkedList(std::size_t capacity)
    : DataStructure("Linked List", capacity) {}

void BoundedLinkedList::insertHead(int value) {
    if (value < -99999 || value > 99999) {
        throw ValidationException("Value must be an integer between -99999 and 99999.");
    }
    if (isFull()) {
        throw OverflowException("Linked List Overflow: capacity (" +
                                std::to_string(capacity_) + ") reached.");
    }
    auto node = std::make_unique<Node>(value);
    node->next = std::move(head_);
    head_ = std::move(node);
    ++count_;
}

void BoundedLinkedList::insertTail(int value) {
    if (value < -99999 || value > 99999) {
        throw ValidationException("Value must be an integer between -99999 and 99999.");
    }
    if (isFull()) {
        throw OverflowException("Linked List Overflow: capacity (" +
                                std::to_string(capacity_) + ") reached.");
    }
    auto node = std::make_unique<Node>(value);
    if (!head_) {
        head_ = std::move(node);
    } else {
        Node* curr = head_.get();
        while (curr->next) {
            curr = curr->next.get();
        }
        curr->next = std::move(node);
    }
    ++count_;
}

int BoundedLinkedList::removeHead() {
    if (isEmpty()) {
        throw UnderflowException("Linked List Underflow: cannot remove from an empty list.");
    }
    int val = head_->value;
    head_ = std::move(head_->next);
    --count_;
    return val;
}

bool BoundedLinkedList::removeValue(int value) {
    if (isEmpty()) {
        throw UnderflowException("Linked List Underflow: list is empty.");
    }
    if (head_->value == value) {
        head_ = std::move(head_->next);
        --count_;
        return true;
    }
    Node* prev = head_.get();
    while (prev->next) {
        if (prev->next->value == value) {
            prev->next = std::move(prev->next->next);
            --count_;
            return true;
        }
        prev = prev->next.get();
    }
    return false;
}

int BoundedLinkedList::findIndex(int value) const {
    if (isEmpty()) {
        throw UnderflowException("Linked List is empty.");
    }
    int idx = 0;
    const Node* curr = head_.get();
    while (curr) {
        if (curr->value == value) {
            return idx;
        }
        curr = curr->next.get();
        ++idx;
    }
    return -1;
}

void BoundedLinkedList::loadSample() {
    clear();
    for (int v : {12, 27, 44, 68}) {
        insertTail(v);
    }
}

std::size_t BoundedLinkedList::size() const noexcept {
    return count_;
}

bool BoundedLinkedList::isEmpty() const noexcept {
    return count_ == 0;
}

bool BoundedLinkedList::isFull() const noexcept {
    return count_ >= capacity_;
}

void BoundedLinkedList::clear() {
    while (head_) {
        head_ = std::move(head_->next);
    }
    count_ = 0;
}

void BoundedLinkedList::setCapacity(std::size_t newCapacity) {
    DataStructure::setCapacity(newCapacity);
}

DataStructureState BoundedLinkedList::state() const {
    DataStructureState s;
    s.name = name_;
    s.size = count_;
    s.capacity = capacity_;
    s.isEmpty = isEmpty();
    s.isFull = isFull();
    s.hasTopOrFront = !isEmpty();
    s.topOrFrontValue = s.hasTopOrFront ? head_->value : 0;
    const Node* curr = head_.get();
    while (curr) {
        s.elements.push_back(curr->value);
        curr = curr->next.get();
    }
    return s;
}

std::ostream& operator<<(std::ostream& os, const DataStructure& ds) {
    const DataStructureState st = ds.state();
    os << st.name << " (" << st.size << "/" << st.capacity << "): ["
       << utils::joinValues(st.elements) << "]";
    return os;
}

} // namespace dsa
