
#ifndef RDMA_CONNECTION_H_
#define RDMA_CONNECTION_H_ 1

#include <boost/shared_ptr.hpp>
#include <string>

namespace rdma {

class RdmaConnection{
public:
    /**
     *  deconstructor
     */
    ~RdmaConnection() {}
    //初始化相关资源
    //建立连接
    int preConn();
    int Open();
    int read();
    int recv();
    int Close();


private:
    /**
     *count 表示数
     *num表示第几
     *num_of_xxx 也可以表示个数，但不用这种方法
     **/

    int                 sock_fd_; //socket描述符，在使用ibv接口的时候用，用于传递QP信息与同步
    int                 port;

    char                *ib_dev_name;//设备名，由于可能多设备，才需要这个字段。现阶段不用,默认为空

    char                *server_ip_;  //由命令行参数得到的IP地址。暂时没有必要做ip地址合法性检测，也不是传输层的功能所在。

    int                 connection_type_; // 连接类型，包括RC、UC等
    int                 _qp_count_;
    int                 use_event;  //使用事件系行为，暂时先不用

    int                 use_srq; //使用srq(共享请求队列)，暂时不用，以后做性能分析的话，应该需要。

    MachineType         machine; //是server端还是client端，由于没有双工，二者必居其一。如果未来测试双工模式，需要考虑扩展性
    VerbType            verb; //send、write、read、atomic等中的一种，现在只测试send（因为类似于socket的行为）

    int                 use_rdma_cm; //使用rdma_cm QP做通信通道，为0则是ibverbs，默认为1
    int                 retry_count_;  //连接的重试次数，default = 0
};

// namespace rdma

#endif // #ifndef _RDMACONNECTION_H_