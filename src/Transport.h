#ifndef RDMA_TRANSPORT_H_
#define RDMA_TRANSPORT_H_ 1

/* The type of the machine ( server or client actually). */
enum MachineType { UNCHOSEN = -1, SERVER , CLIENT} ;

/* The Verb of the benchmark. */
enum VerbType { SEND , WRITE, READ, ATOMIC };

namespace rdma{
public:
    int Open();
    int Close();
private:

    /**
     *_num表示第几
     *num_of_xxx 也可以表示个数，但不用这种方法，为了与rdma库的设计保持一致.
     *xx_count 表示个数
     **/
    /** Host to connect to */
    std::string host_;

    struct rdma_cm_id *cm_id_; //cm_id是连接的唯一标识符，相当于sock_fd;
    int                 sock_fd_; //socket描述符，在使用ibv接口的时候用，用于传递QP信息与同步，暂时不用
    int                 port_;

    char                *ib_dev_name;//设备名，由于可能多设备，才需要这个字段。现阶段不用,默认为空

    char                *ip_addr_;  //由命令行参数得到的IP地址。暂时没有必要做ip地址合法性检测，也不是传输层的功能所在。

    int                 connection_type_; // 连接类型，包括RC、UC等
    int                 _qp_count_;
    int                 use_event;  //使用事件系行为，暂时先不用

    int                 use_srq; //使用srq(共享请求队列)，暂时不用，以后做性能分析的话，应该需要。
    int                 connection_type_; // 连接类型，包括RC、UC等
    int                 _qp_count_;
    int                 use_event;  //使用事件行为，暂时先不用

    int                 use_srq; //使用srq(共享请求队列)，暂时不用，以后做性能分析的话，应该需要。

    MachineType         machine; //是server端还是client端，由于没有双工，二者必居其一。如果未来测试双工模式，需要考虑扩展性
    VerbType            verb; //send、write、read、atomic等中的一种，现在只测试send（因为类似于socket的行为）

    int                 use_rdma_cm; //使用rdma_cm QP做通信通道，为0则是ibverbs，默认为1
    int                 retry_count_;  //连接的重试次数，default = 0
    int32_t            mtu_    = 0;
}


#endif //RDMA_TRANSPORT_H_





struct __gf_rdma_ctx {
        gf_rdma_device_t          *device;
        struct rdma_event_channel *rdma_cm_event_channel;
        pthread_t                  rdma_cm_thread;
        pthread_mutex_t            lock;
        int32_t                    dlcount;
};
typedef struct __gf_rdma_ctx gf_rdma_ctx_t;