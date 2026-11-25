#pragma once

#include "dsa/Types.hpp"

#include <cstddef>
#include <memory>
#include <ostream>
#include <queue>
#include <stack>
#include <string>
#include <vector>

namespace dsa {

class DataStructure {
public:
    DataStructure(std::string name, std::size_t capacity);
    virtual ~DataStructure() = default;

    const std::string& name() const noexcept { return name_; }
    std::size_t capacity() const noexcept { return capacity_; }

    virtual std::size_t size() const noexcept = 0;
    virtual bool isEmpty() const noexcept = 0;
    virtual bool isFull() const noexcept = 0;
    virtual void clear() = 0;
    virtual void setCapacity(std::size_t newCapacity);
    virtual DataStructureState state() const = 0;

protected:
    static void validateCapacity(std::size_t capacity);
    std::string name_;
    std::size_t capacity_;
};

class BoundedStack final : public DataStructure {
public:
    explicit BoundedStack(std::size_t capacity = 8);
    ~BoundedStack() override = default;

    // Function overloading: single push vs batch push
    void push(int value);
    void push(const std::vector<int>& values);
    int pop();
    int peek() const;

    // Operator overloading: stack += value pushes onto the stack
    BoundedStack& operator+=(int value);

    std::size_t size() const noexcept override;
    bool isEmpty() const noexcept override;
    bool isFull() const noexcept override;
    void clear() override;
    void setCapacity(std::size_t newCapacity) override;
    DataStructureState state() const override;

private:
    std::vector<int> snapshotBottomToTop() const;
    std::stack<int> stack_;
};

class BoundedQueue final : public DataStructure {
public:
    explicit BoundedQueue(std::size_t capacity = 8);
    ~BoundedQueue() override = default;

    // Function overloading: single enqueue vs batch enqueue
    void enqueue(int value);
    void enqueue(const std::vector<int>& values);
    int dequeue();
    int front() const;

    // Operator overloading: queue += value enqueues an element
    BoundedQueue& operator+=(int value);

    std::size_t size() const noexcept override;
    bool isEmpty() const noexcept override;
    bool isFull() const noexcept override;
    void clear() override;
    void setCapacity(std::size_t newCapacity) override;
    DataStructureState state() const override;

private:
    std::vector<int> snapshotFrontToRear() const;
    std::queue<int> queue_;
};

class BoundedLinkedList final : public DataStructure {
public:
    explicit BoundedLinkedList(std::size_t capacity = 10);
    ~BoundedLinkedList() override = default;

    void insertHead(int value);
    void insertTail(int value);
    int removeHead();
    bool removeValue(int value);
    int findIndex(int value) const;
    void loadSample();

    std::size_t size() const noexcept override;
    bool isEmpty() const noexcept override;
    bool isFull() const noexcept override;
    void clear() override;
    void setCapacity(std::size_t newCapacity) override;
    DataStructureState state() const override;

private:
    struct Node {
        int value;
        std::unique_ptr<Node> next;
        explicit Node(int v) : value(v) {}
    };

    std::unique_ptr<Node> head_;
    std::size_t count_ = 0;
};

std::ostream& operator<<(std::ostream& os, const DataStructure& ds);

} // namespace dsa
