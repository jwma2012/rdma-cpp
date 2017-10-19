/**
 *  Copyright (c) 2017 by jwma
 */
#ifndef RDMA_API_TRANSPORT_H_
#define RDMA_API_TRANSPORT_H_


namespace rdma {

/**
 * \brief 传输消息的基类，定义接口
 *
 */
class Transport {
 public:

  /**
   * Virtual deconstructor. do nothing. use Close for real close
   */
  virtual ~Transport() { }

  /**
   * Whether this transport is open.
   */
  virtual bool IsOpen() { return false; }

  /**
   * \brief start van
   *
   * must call it before calling Send
   *
   * it initalizes all connections to other nodes.  start the receiving
   * threads, which keeps receiving messages. if it is a system
   * control message, give it to postoffice::manager, otherwise, give it to the
   * accoding app.
   */
  virtual void Open();
  /**
   * \brief send a message, It is thread-safe
   * \return the number of bytes sent. -1 if failed
   */
  int Send(const Message& msg);
  /**
   * \brief return my node
   */
  const Node& my_node() const {
    CHECK(ready_) << "call Start() first";
    return my_node_;
  }
  /**
   * \brief stop van
   * stop receiving threads
   */
  virtual void Close();
  /**
   * \brief get next available timestamp. thread safe
   */
  int GetTimestamp() { return timestamp_++; }
  /**
   * \brief whether it is ready for sending. thread safe
   */
  bool IsReady() { return ready_; }

 protected:
  /**
   * \brief connect to a node
   */
  virtual void Connect(const Node& node) = 0;
  /**
   * \brief bind to my node
   * do multiple retries on binding the port. since it's possible that
   * different nodes on the same machine picked the same port
   * \return return the port binded, -1 if failed.
   */
  virtual int Bind(const Node& node, int max_retry) = 0;
  /**
   * \brief block until received a message
   * \return the number of bytes received. -1 if failed or timeout
   */
  virtual int RecvMsg(Message* msg) = 0;
  /**
   * \brief send a mesage
   * \return the number of bytes sent
   */
  virtual int SendMsg(const Message& msg) = 0;
  /**
   * \brief pack meta into a string
   */
  void PackMeta(const Meta& meta, char** meta_buf, int* buf_size);
  /**
   * \brief unpack meta from a string
   */
  void UnpackMeta(const char* meta_buf, int buf_size, Meta* meta);

  Node scheduler_;
  Node my_node_;
  bool is_scheduler_;

 private:
  /** thread function for receving */
  void Receiving();
  /** thread function for heartbeat */
  void Heartbeat();
  /** whether it is ready for sending */
  std::atomic<bool> ready_{false};
  std::atomic<size_t> send_bytes_{0};
  size_t recv_bytes_ = 0;
  int num_servers_ = 0;
  int num_workers_ = 0;
  /** the thread for receiving messages */
  std::unique_ptr<std::thread> receiver_thread_;
  /** the thread for sending heartbeat */
  std::unique_ptr<std::thread> heartbeat_thread_;
  std::vector<int> barrier_count_;
  /** msg resender */
  Resender* resender_ = nullptr;
  int drop_rate_ = 0;
  std::atomic<int> timestamp_{0};
  DISALLOW_COPY_AND_ASSIGN(Van);
};


/**
 * Generic factory class to make an input and output transport out of a
 * source transport. Commonly used inside servers to make input and output
 * streams out of raw clients.
 *
 */
class TTransportFactory {
public:
  TTransportFactory() {}

  virtual ~TTransportFactory() {}

  /**
   * Default implementation does nothing, just returns the transport given.
   */
  virtual boost::shared_ptr<TTransport> getTransport(boost::shared_ptr<TTransport> trans) {
    return trans;
  }
};

}  // namespace rdma
#endif  // RDMA_API_TRANSPORT_H_

