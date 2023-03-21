#include "pingpong_buffer.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

PingpongBuffer::PingpongBuffer() :
        producer_buffer_in_use_(-1),
        producer_buffer_used_(-1),
        consumer_buffer_in_use_(-1),
        buffer_0_updated_(false),
        buffer_1_updated_(false),
        buffer_len_(0),
        buffer_0_(nullptr),
        buffer_1_(nullptr) {
}

PingpongBuffer::~PingpongBuffer() {
    if (buffer_0_ != nullptr) {
        free(buffer_0_);
        buffer_0_ = nullptr;
    }
    if (buffer_1_ != nullptr) {
        free(buffer_1_);
        buffer_1_ = nullptr;
    }
}

int PingpongBuffer::CreateBuffer(int len) {
    FreeBuffer();
    buffer_0_ = malloc(len);
    buffer_1_ = malloc(len);
    if (buffer_0_ == nullptr || buffer_1_ == nullptr) {
        FreeBuffer();
        return -1;
    }
    return 0;
}

void PingpongBuffer::UpdateBuffer(void* data, int len) {
    int buf_index = GetProducerAvaliableBuffer();
    printf("Producer Buffer In Use %d\n", buf_index);
    if (buf_index == 0) {
        // usleep(100000);
        UpdateBuffer0(data, len);
        printf("UpdatedBuffer0\n");
    } else {
        // usleep(100000);
        UpdateBuffer1(data, len);
        printf("UpdatedBuffer1\n");
    }
}

void PingpongBuffer::GetBuffer(void** data, int* len) {
    int buf_index = GetConsumerAvaliableBuffer();
    printf("GetConsumerAvaliableBuffer: %d\n", buf_index);
    if (buf_index == 0) {
        *data = buffer_0_;
        *len = buffer_len_;
    } else if (buf_index == 1){
        *data = buffer_1_;
        *len = buffer_len_;
    } else {
        *data = nullptr;
        *len = 0;
    }
}

void PingpongBuffer::GetBufferBlock(void** data, int* len, int timeout) {
    int buf_index = GetConsumerAvaliableBuffer();
    printf("GetConsumerAvaliableBuffer: %d\n", buf_index);
    if (buf_index == 0) {
        *data = buffer_0_;
        *len = buffer_len_;
    } else if (buf_index == 1){
        *data = buffer_1_;
        *len = buffer_len_;
    } else {
        std::unique_lock<mutex> lock(buffer_mutex_);
        buffer_condition_variable_.wait_for(lock, std::chrono::milliseconds(timeout));
        *data = nullptr;
        *len = 0;
        printf("notify: \n");
    }
}

void PingpongBuffer::BufferDone() {
    std::unique_lock<mutex> lock(buffer_mutex_);
    if (consumer_buffer_in_use_ == 0) {
        buffer_0_updated_ = false;
    } else if (consumer_buffer_in_use_ == 1) {
        buffer_1_updated_ = false;
    }
    printf("BufferDone: %d\n", consumer_buffer_in_use_);
    consumer_buffer_in_use_ = -1;
}

int PingpongBuffer::GetProducerAvaliableBuffer() {
    std::unique_lock<mutex> lock(buffer_mutex_);
    if (producer_buffer_used_ == -1)  { // 没写过数据
        producer_buffer_in_use_ = 0;
        return 0;
    }

    if (producer_buffer_used_ == 0) {
        if (consumer_buffer_in_use_ != 1 ) { // buffer_1 没有正在被使用
            producer_buffer_in_use_ = 1;
            // 往buffer_1复制数据
        } else {
            producer_buffer_in_use_ = 0;
            // 往buffer_0复制数据
        }
    } else {
        if (consumer_buffer_in_use_ != 0 ) { // buffer_0 没有正在被使用
            producer_buffer_in_use_ = 0;
            // 往buffer_0复制数据
        } else {
            producer_buffer_in_use_ = 1;
            // 往buffer_1复制数据
        }
    }
    return producer_buffer_in_use_;
}

int PingpongBuffer::GetConsumerAvaliableBuffer() {
    std::unique_lock<mutex> lock(buffer_mutex_);
    if (producer_buffer_used_ == 0) {
        if (producer_buffer_in_use_ != 0) { // producer_buffer_in_use_ == 1 或者 -1
            if (buffer_0_updated_) {
                consumer_buffer_in_use_ = 0;
                // 处理 buffer_0_
            } else {
                printf("A\n");
                return -1;
            }
        } else { // producer_buffer_in_use_ == 0
            if (buffer_1_updated_) {
                consumer_buffer_in_use_ = 1;
            } else {
                printf("b\n");
                return -1;
            }
        }
        return consumer_buffer_in_use_;
    } else if(producer_buffer_used_ == 1){
        if (producer_buffer_in_use_ != 1) { // producer_buffer_in_use_ == 0 或者 -1
            if (buffer_1_updated_) {
                consumer_buffer_in_use_ = 1;
            } else {
                printf("c\n");
                return -1;
            }
        } else { // producer_buffer_in_use_ == 0
            if (buffer_0_updated_) {
                consumer_buffer_in_use_ = 0;
            } else {
                printf("D\n");
                return -1;
            }
        }
        return consumer_buffer_in_use_;
    }
                printf("E\n");
    return -1;
}

void PingpongBuffer::UpdateBuffer0(void* data, int len) {
    memcpy(buffer_0_, data, len);
    std::unique_lock<mutex> lock(buffer_mutex_);
    producer_buffer_used_ = 0;
    producer_buffer_in_use_ = -1;
    buffer_0_updated_ = true;
    buffer_condition_variable_.notify_all();
}

void PingpongBuffer::UpdateBuffer1(void* data, int len) {
    memcpy(buffer_1_, data, len);
    std::unique_lock<mutex> lock(buffer_mutex_);
    producer_buffer_used_ = 1;
    producer_buffer_in_use_ = -1;
    buffer_1_updated_ = true;
    buffer_condition_variable_.notify_all();
}

void PingpongBuffer::FreeBuffer() {
    if (buffer_0_ != nullptr) {
        free(buffer_0_);
        buffer_0_ = nullptr;
    }
    if (buffer_1_ != nullptr) {
        free(buffer_1_);
        buffer_1_ = nullptr;
    }
}
