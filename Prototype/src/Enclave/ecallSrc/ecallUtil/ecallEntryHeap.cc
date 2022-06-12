/**
 * @file EcallEntryHeap.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief optimized the heap index
 * @version 0.1
 * @date 2021-08-17
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/ecallEntryHeap.h"

inline bool CompFreq(uint32_t freq1, uint32_t freq2) {
    return (freq1 < freq2);
}

/**
 * @brief Construct a new New Ecall Entry Heap object
 * 
 */
EcallEntryHeap::EcallEntryHeap() {
    ;
}

/**
 * @brief Destroy the New Ecall Entry Heap object
 * 
 */
EcallEntryHeap::~EcallEntryHeap() {
    ;
}

/**
 * @brief Swap up the entry
 * 
 * @param idx 
 * @return uint32_t 
 */
uint32_t EcallEntryHeap::SwapUp(uint32_t idx) {
    uint32_t parent;
    for (parent = this->ParentIdx(idx);
        idx > 0 && CompFreq(_heap[idx]->second.chunkFreq, _heap[parent]->second.chunkFreq);
        idx = parent, parent = this->ParentIdx(idx)) {
        std::swap(_heap[idx], _heap[parent]);
        _heap[idx]->second.idx = idx; 
    }
    return idx;
}

/**
 * @brief Swap down the entry
 * 
 * @param idx 
 * @return uint32_t 
 */
uint32_t EcallEntryHeap::SwapDown(uint32_t idx) {
    uint32_t child;
    for (child = this->ChildIdx(idx); child < _heap.size(); idx = child, 
        child = this->ChildIdx(idx)) {
        child += ((child + 1 < _heap.size()) && 
            CompFreq(_heap[child + 1]->second.chunkFreq, _heap[child]->second.chunkFreq));
        if (!CompFreq(_heap[child]->second.chunkFreq, _heap[idx]->second.chunkFreq)) {
            break;
        }
        std::swap(_heap[child], _heap[idx]);
        _heap[idx]->second.idx = idx; 
    }
    return idx;
}

/**
 * @brief get the entry of the top element
 * 
 * @return uint32_t the frequency of the top item
 */
uint32_t EcallEntryHeap::TopEntry() const {
    return _heap[0]->second.chunkFreq;
}

/**
 * @brief pop the element off the heap
 * 
 */
void EcallEntryHeap::Pop() {
    auto t = _heap[0];
    // copy last element into first position, drop last entry
    _heap[0] = _heap[_heap.size() - 1];
    _heap.pop_back();
    _index.erase(t->first);
    uint32_t idx = this->SwapDown(0);
    _heap[idx]->second.idx = idx; // update the new position
    return ;
}

/**
 * @brief Get the size object
 * 
 * @return size_t the heap size
 */
size_t EcallEntryHeap::Size() const {
    return _heap.size();
}

/**
 * @brief add an element in the heap
 * 
 * @param key the fp 
 * @param value the value
 */
void EcallEntryHeap::Add(const string& key, const HeapItem_t& value) {
    auto tmpIt = _index.insert({key, value}).first;
    _heap.push_back(tmpIt);
    uint32_t idx = this->SwapUp(static_cast<uint32_t>(_heap.size() - 1));
    _index[key].idx = idx;
    return ;
}

/**
 * @brief update the frequency of an element
 * 
 * @param key the fp 
 * @param freq the new freq
 */
void EcallEntryHeap::Update(const string&key, uint32_t freq) {
    uint32_t idx = _index[key].idx;
    _heap[idx]->second.chunkFreq = freq;
    // try swap up/down into place
    idx = SwapUp(idx);
    idx = SwapDown(idx);
    _index[key].idx = idx;
    return ;
}

/**
 * @brief check if the heap contains an element
 * 
 * @param key the fp
 * @return true exist
 * @return false non-exist
 */
bool EcallEntryHeap::Contains(const string& key) const {
    return (_index.find(key) != _index.end());
}

/**
 * @brief Get the Priority object
 * 
 * @param key the key of the required element
 * @return the heap item ptr
 */
HeapItem_t* EcallEntryHeap::GetPriority(const string& key) {
    return &_index[key];
}

/**
 * @brief Set the Heap Size object
 * 
 * @param heapSize the heap size
 */
void EcallEntryHeap::SetHeapSize(size_t heapSize) {
    _heap.reserve(heapSize);
    _index.reserve(heapSize); // avoid re-hashing
    return ;
}