
/* represents one communication peer, two per transport_t */
struct __gf_rdma_peer {
        rpc_transport_t   *trans;
        struct rdma_cm_id *cm_id;
        struct ibv_qp     *qp;
        pthread_t          rdma_event_thread;
        char               quota_set;

        int32_t recv_count;
        int32_t send_count;
        int32_t recv_size;
        int32_t send_size;

        int32_t                        quota;
        union {
                struct list_head       ioq;
                struct {
                        gf_rdma_ioq_t *ioq_next;
                        gf_rdma_ioq_t *ioq_prev;
                };
        };

        /* QP attributes, needed to connect with remote QP */
        int32_t               local_lid;
        int32_t               local_psn;
        int32_t               local_qpn;
        int32_t               remote_lid;
        int32_t               remote_psn;
        int32_t               remote_qpn;
};
typedef struct __gf_rdma_peer gf_rdma_peer_t;
