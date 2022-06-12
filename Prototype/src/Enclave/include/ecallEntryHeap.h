/**
 * @file EcallEntryHeap.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief optimized the heap index
 * @version 0.1
 * @date 2021-08-17
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef ECALL_NEW_ENTRY_HEAP_H
#define ECALL_NEW_ENTRY_HEAP_H

#include "commonEnclave.h"
#include "functional"

class EcallEntryHeap {
    private:
        /**
         * @brief get the parent index
         * 
         * @param idx input index
         * @return uint32_t parent index
         */
        inline uint32_t ParentIdx(uint32_t idx) {
            return (idx - 1) / 2;
        }

        /**
         * @brief get the child index
         * 
         * @param idx input index
         * @return uint32_t child index
         */
        inline uint32_t ChildIdx(uint32_t idx) {
            return idx * 2 + 1;
        }

        /**
         * @brief Swap up the entry
         * 
         * @param idx 
         * @return uint32_t 
         */
        uint32_t SwapUp(uint32_t idx);

        /**
         * @brief Swap down the entry
         * 
         * @param idx 
         * @return uint32_t 
         */
        uint32_t SwapDown(uint32_t idx);

    public:
        // the vector to store top-k index reference
        vector<unordered_map<string, HeapItem_t>::iterator> _heap; 

        // the index
        unordered_map<string, HeapItem_t> _index;

        /**
         * @brief Construct a new New Ecall Entry Heap object
         * 
         */
        EcallEntryHeap();

        /**
         * @brief Destroy the New Ecall Entry Heap object
         * 
         */
        ~EcallEntryHeap();

        /**
         * @brief get the entry of the top element
         * 
         * @return uint32_t the frequency of the top item
         */
        uint32_t TopEntry() const;

        /**
         * @brief pop the element off the heap
         * 
         */
        void Pop();

        /**
         * @brief Get the size object
         * 
         * @return size_t the heap size
         */
        size_t Size() const;

        /**
         * @brief add an element in the heap
         * 
         * @param key the fp 
         * @param value the value
         */
        void Add(const string& key, const HeapItem_t& value);

        /**
         * @brief update the frequency of an element
         * 
         * @param key the fp 
         * @param freq the new freq
         */
        void Update(const string&key, uint32_t freq);

        /**
         * @brief check if the heap contains an element
         * 
         * @param key the fp
         * @return true exist
         * @return false non-exist
         */
        bool Contains(const string& key) const;

        /**
         * @brief Get the Priority object
         * 
         * @param key the key of the required element
         * @return the heap item ptr
         */
        HeapItem_t* GetPriority(const string& key);

        /**
         * @brief Set the Heap Size object
         * 
         * @param heapSize the heap size
         */
        void SetHeapSize(size_t heapSize);
};

#endif