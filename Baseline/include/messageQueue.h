/**
 * @file messageQueue.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief 
 * @version 0.1
 * @date 2019-12-31
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#ifndef BASICDEDUP_MESSAGEQUEUE_h
#define BASICDEDUP_MESSAGEQUEUE_h

#include "configure.h"
#include "chunkStructure.h"
#include "messageQueue/readerwriterqueue.h"
#include <boost/atomic.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>


template <class T>
class MessageQueue {
    private:
        // dynamic size queue
        // boost::lockfree::queue<T, boost::lockfree::fixed_sized<false>> lockFreeQueue_{0};
        // boost::lockfree::queue<T, boost::lockfree::capacity<QUEUE_SIZE>> lockFreeQueue_;
        // moodycamel::ConcurrentQueue<T>* lockFreeQueue_;
        moodycamel::ReaderWriterQueue<T>* lockFreeQueue_;

    public:
        // to show whether the whole process is done
        boost::atomic<bool> done_;
        
        /**
         * @brief Construct a new Message Queue object
         * 
         */
        MessageQueue(uint32_t maxQueueSize) {
            // testpointer = new boost::lockfree::queue<T>(1);
            // lockFreeQueue_ = new moodycamel::ConcurrentQueue<T>(QUEUE_SIZE);
            lockFreeQueue_ = new moodycamel::ReaderWriterQueue<T>(maxQueueSize);
            done_ = false;
        }

        /**
         * @brief Destroy the Message Queue object
         * 
         */
        ~MessageQueue() {
            bool flag = IsEmpty();
            if (flag) {
	            delete lockFreeQueue_;
            } else {
                fprintf(stderr, "MessageQueue: Queue is not empty.\n");
                exit(EXIT_FAILURE);
            }
            // fprintf(stderr, "Destory the message queue.\n");    
        }

        /**
         * @brief push data to the queue
         * 
         * @param data the original data
         * @return true success
         * @return false fails
         */
        bool Push(T& data) {
            // while (!lockFreeQueue_.push(data)) {
            while (!lockFreeQueue_->try_enqueue(data)) {
                ;
            }
            return true;
        }

        /**
         * @brief pop data from the queue 
         * 
         * @param data the original data
         * @return true success
         * @return false fails
         */
        bool Pop(T& data) {
            // return lockFreeQueue_.pop(data);
            return lockFreeQueue_->try_dequeue(data);
        }

        /**
         * @brief Set the Job Done Flag object
         * 
         * @return true success
         * @return false fails
         */
        void SetJobDoneFlag() {
            done_ = true;
        }

        /**
         * @brief Decide whether the queue is empty
         * 
         * @return true success
         * @return false fails
         */
        bool IsEmpty() {
            // return lockFreeQueue_.empty();
            size_t count = lockFreeQueue_->size_approx();
            if (count == 0) {
                return true;
            } else {
                return false;
            }
        }
};

#endif // BASICDEDUP_MESSAGEQUEUE_h
