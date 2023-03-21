#ifndef PINGPONG_BUFFER_H__
#define PINGPONG_BUFFER_H__
#include <mutex>
#include <condition_variable>

using std::mutex;
using std::condition_variable;

class PingpongBuffer {
 public:
    PingpongBuffer();
    ~PingpongBuffer();

    int CreateBuffer(int len);

    void UpdateBuffer(void* data, int len);

    // 获取更新的buffer, 如果没有数据更新返回空指针
    void GetBuffer(void** data, int* len);
    // 获取更新的buffer, 如果没有数据更新会阻塞,ms
    void GetBufferBlock(void** data, int* len, int timeout);
    // 通知buffer使用完
    void BufferDone();

 private:
    int GetProducerAvaliableBuffer();
    int GetConsumerAvaliableBuffer();
    void UpdateBuffer0(void* data, int len);
    void UpdateBuffer1(void* data, int len);

    void FreeBuffer();

    mutex buffer_mutex_;
    condition_variable buffer_condition_variable_;
    int producer_buffer_in_use_;
    int producer_buffer_used_;
    int consumer_buffer_in_use_;
    bool buffer_0_updated_;
    bool buffer_1_updated_;

    int buffer_len_;
    void* buffer_0_;
    void* buffer_1_;
};

#endif // PINGPONG_BUFFER_H__
