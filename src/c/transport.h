#ifndef RDMA_TRANSPORT_H_
#define RDMA_TRANSPORT_H_ 1


#define DEFAULT_BUF_DEPTH 4096

/* Outstanding reads for "read" verb only. */
#define MAX_SEND_SGE            (1)
#define MAX_RECV_SGE            (1)

/* The type of the machine ( server or client actually). */
enum MachineType { UNCHOSEN = -1, SERVER , CLIENT} ;

/* The Verb of the benchmark. */
enum VerbType { SEND , WRITE, READ, ATOMIC };

enum ConnectionType { RC, UD, UC};


/* Macro for allocating. */
#define ALLOCATE(var,type,size)                                     \
{ if((var = (type*)malloc(sizeof(type)*(size))) == NULL)        \
        { fprintf(stderr," Cannot Allocate\n"); exit(1);}}

#define TIMEOUT_IN_MS (2000)

#define PAGE_SIZE (sysconf(_SC_PAGESIZE))
#define BUF_SIZE (4096*1024)

struct rdma_context {
    struct rdma_event_channel       *cm_event_channel;
    struct rdma_cm_id           *cm_id;
    struct ibv_context          *context;
    struct ibv_comp_channel         *channel;
    struct ibv_pd               *pd;
    struct ibv_mr               **mr;
    struct ibv_cq               *send_cq;
    struct ibv_cq               *recv_cq;
    void                    **buf;
    struct ibv_ah               **ah;
    struct ibv_qp               **qp;
    struct ibv_srq              *srq;
    struct ibv_sge              *sge_list;
    struct ibv_sge              *recv_sge_list;
    struct ibv_send_wr          *wr;
    struct ibv_recv_wr          *rwr;
    struct ibv_port_attr port_attr; /* IB port attributes */
    int connected;
}
tpyedef struct rdma_context rdma_ctx_t;

struct rdma_param {
    int         qp_count;
    int             port;
    char                *ib_devname;
    char                *ip_addr;
    int              ib_port;
    int                     mtu;
    //enum ibv_mtu            curr_mtu;
    int gid_idx;
    int retry_count;
    int use_event;
    int use_srq;
    MachineType         machine; //是server端还是client端，由于没有双工，二者必居其一。如果未来测试双工模式，需要考虑扩展性
    VerbType            verb; //send、write、read、atomic等中的一种，现在只测试send（因为类似于socket的行为）

    ConnectionType connection_type;//连接类型
}

typedef rdma_param rdma_param_t;

//global
static rdma_ctx_t *rdma_ctx = NULL;
static rdma_param_t param
{
    1,
    19875, /* tcp_port */
    NULL, /* dev_name */
    NULL, /* server_name(ip_addr) */
    1, /* ib_port */
    4096,
    -1, /* gid_idx */
    0,
    0,
    0,
    UNCHOSEN,
    SEND,
    RC
};
static rdma_param_t *config = &param;

int listen();

#endif