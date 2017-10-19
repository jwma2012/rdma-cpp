#include "transport.h"

/*
struct rdma_ctx {
        fs_rdma_device_t          *device;
        struct rdma_event_channel *rdma_cm_event_channel;
        pthread_t                  rdma_cm_thread;
        pthread_mutex_t            lock;
};

tpyedef struct rdma_ctx rdma_ctx_t;

*/

/**
 *函数功能：获取设备列表(用于多IB设备的情况)(实验室环境下不存在这种情况)
 *暂时不需要被调用
 *(留用以便将来扩展)
 *ib_devname可以为NULL
 **/
static struct ibv_device* fs_rdma_find_dev(const char *ib_devname)
{
    int num_of_device;
    struct ibv_device **dev_list;
    struct ibv_device *ib_dev = NULL;

    dev_list = ibv_get_device_list(&num_of_device);
    if (!dev_list)
    {
        fprintf(stderr, "failed to get IB devices list\n");
        return NULL;
    }

    /* if there isn't any IB device in host */
    if (num_of_device <= 0) {
        fprintf(stderr," Did not detect devices \n");
        fprintf(stderr," If device exists, check if driver is up\n");
        return NULL;
    }

    fprintf(stdout, "found %d device(s)\n", num_of_device);
    if (!ib_devname) {
        ib_dev = dev_list[0];
        if (!ib_dev) {
            fprintf(stderr, "No IB devices found\n");
            return NULL;
        }
        fprintf(stdout, "device not specified, using first one found: %s\n", strdup(ibv_get_device_name(dev_list[0])));
    } else {
        for (; (ib_dev = *dev_list); ++dev_list)
            if (!strcmp(ibv_get_device_name(ib_dev), ib_devname))
                break;
        if (!ib_dev)
            fprintf(stderr, "IB device %s not found\n", ib_devname);
    }

    /*
    在open设备之后需要做的工作
    ibv_free_device_list(dev_list);
    dev_list = NULL;
    ib_dev = NULL;
    */
    return ib_dev;

}

int fs_rdma_create_ctx (void)
{
        //rdma_ctx_t *rdma_ctx = NULL;
        int ret = 0;

        rdma_ctx = malloc(sizeof (rdma_ctx_t));
        if (rdma_ctx == NULL) {
            ret = -1;
            return ret;
        }
        //pthread_mutex_init (&rdma_ctx->lock, NULL);
        rdma_ctx->cm_event_channel = rdma_create_event_channel ();
        if (rdma_ctx->cm_event_channel == NULL) {
                fprintf(stderr, "%s", "rdma_cm event channel creation failed");
            ret = -1;
            return ret;
        }

//创建rdma_cm_id，作为连接的标识符
         enum rdma_port_space port_space = (config->connection_type == UD) ? RDMA_PS_UDP : RDMA_PS_TCP;
        if (rdma_create_id(rdma_ctx->cm_event_channel,&(rdma_ctx->cm_id),NULL,port_space)) {
            fprintf(stderr,"rdma_create_id failed\n");
            ret = -1;
            return ret;
        }

        ALLOCATE(ctx->qp, struct ibv_qp*, config->qp_count);
        ALLOCATE(ctx->mr, struct ibv_mr*, config->qp_count);
        ALLOCATE(ctx->buf, void* , config->qp_count);


        return ret;
}

int fs_rdma_destroy_ctx (void)
{
    rdma_destroy_id(rdma_ctx->cm_id);
    rdma_destroy_event_channel(ctx->cm_channel);
    return 0;
}

static void *fs_rdma_cm_event_handler (struct rdma_event_channel *event_channel)
{
        struct rdma_cm_event      *event         = NULL;
        int                        ret           = 0;

        while (1) {
                ret = rdma_get_cm_event (event_channel, &event);
                memcpy(&event_copy, event, sizeof(*event));

                if (ret != 0) {
                        fprintf(stderr, "%s", "rdma_cm_get_event failed");
                        break;
                }

                switch (event->event) {
                case RDMA_CM_EVENT_ADDR_RESOLVED:
                        fs_rdma_cm_handle_addr_resolved (event);
                        break;

                case RDMA_CM_EVENT_ROUTE_RESOLVED:
                        fs_rdma_cm_handle_route_resolved (event);
                        break;

                case RDMA_CM_EVENT_CONNECT_REQUEST:
                        fs_rdma_cm_handle_connect_request (event);
                        break;

                case RDMA_CM_EVENT_ESTABLISHED:
                        fs_rdma_cm_handle_connect_init (event);
                        break;

                case RDMA_CM_EVENT_ADDR_ERROR:
                case RDMA_CM_EVENT_ROUTE_ERROR:
                case RDMA_CM_EVENT_CONNECT_ERROR:
                case RDMA_CM_EVENT_UNREACHABLE:
                case RDMA_CM_EVENT_REJECTED:

                        fprintf(stderr, "cma event %s, error %d\n",
                                rdma_event_str(event->event), event->status);
                        rdma_ack_cm_event (event);
                        event = NULL;
                        fs_rdma_cm_handle_event_error ();
                        continue;

                case RDMA_CM_EVENT_DISCONNECTED:
                        fprintf(stdout, 0, "received disconnect ");
                        rdma_ack_cm_event (event);
                        event = NULL;
                        fs_rdma_cm_handle_disconnect ();
                        continue;

                default:
                        fprintf(stderr, "unhandled event: %s, ignoring",
                                rdma_event_str(event->event));
                        break;
                }

                rdma_ack_cm_event (event);
        }

        return NULL;
}

static int fs_rdma_cm_handle_addr_resolved (struct rdma_cm_event *event)
{
        int ret = 0;
        ret = rdma_resolve_route(event->id, TIMEOUT_IN_MS);
        if (ret != 0) {
            fprintf(stderr, "rdma_resolve_route failed!\n");
            fs_rdma_cm_handle_disconnect ();
            return ret;
        }

        return ret;
}

static int32_t
fs_rdma_create_resources ()
{
        int32_t            ret         = 0;
        char              *device_name = NULL;
        struct ibv_device       *ib_dev = NULL;

        ib_dev = fs_rdma_find_dev(device_name); //不指定设备名，选择找到的第一个

        rdma_ctx->context = ibv_open_device(ib_dev);
        if (!rdma_ctx->context)
        {
            fprintf(stderr, "failed to open device %s\n", (char *)ibv_get_device_name(ibvib_dev));
            ret = -1;
            goto out;
        }

//查询端口
        if (ibv_query_port(rdma_ctx->context, config.ib_port, &rdma_ctx->port_attr))
        {
            fprintf(stderr, "ibv_query_port on port %u failed\n", config.ib_port);
            ret = -1;
            goto out;
        }

//分配保护域
        rdma_ctx->pd = ibv_alloc_pd(rdma_ctx->context);
        if (!rdma_ctx->pd)
        {
            fprintf(stderr, "could not allocate protection domain for device\n");
            ret = -1;
            goto out;
        }

//需要时创建完成事件通道
        if (config->use_event) {
            rdma_ctx->channel = ibv_create_comp_channel(rdma_ctx->context);
            if (!rdma_ctx->channel) {
                fprintf(stderr, "Couldn't create completion channel\n");
                ret = -1;
                goto out;
            }
        }

        struct ibv_device_attr  device_attr = {{0}, };
        ret = ibv_query_device (priv->device->context, &device_attr);
        if (ret != 0) {
                fprintf(stderr,"ibv_query_device failed");
                ret = -1;
                goto out;
        }

//创建请求完成队列
        int cqe_count = DEFAULT_BUF_DEPTH * config->qp_count;//
        cqe_count = (cqe_count > device_attr.max_cqe)
                        ? device_attr.max_cqe : cqe_count;//用于容纳多少个请求
        rdma_ctx->send_cq = ibv_create_cq(ctx->context, cqe_count, NULL, ctx->channel, 0);
        //第三个参数是用户自定义的，需要从事件中获取或者说一直跟着事件的值
        if (!rdma_ctx->send_cq) {
            fprintf(stderr, "Couldn't create CQ\n");
            ret = -1;
            goto out;
        }

        rdma_ctx->recv_cq = ibv_create_cq(ctx->context, cqe_count, NULL, ctx->channel, 0);
        //第三个参数是用户自定义的，需要从事件中获取或者说一直跟着事件的值
        if (!rdma_ctx->send_cq) {
            fprintf(stderr, "Couldn't create CQ\n");
            ret = -1;
            goto out;
        }
        if (config->use_srq){
            struct ibv_srq_init_attr attr = {
                        .attr = {
                                .max_wr = DEFAULT_BUF_DEPTH,
                                .max_sge = 1,
                                .srq_limit = 10
                        }
            };
            rdma_ctx->srq = ibv_create_srq (rdma_ctx->pd, &attr);
            if (!rdma_ctx->srq) {
                fprintf(stderr, "Couldn't create SRQ\n");
                ret = -1;
                goto out;
            }
        }

//创建QP
        int i = 0；
        for (i = 0; i < config->qp_count; i++)
        {
            //如果不用srq的话，rdma_ctx->srq=NULL
            struct ibv_qp_init_attr init_attr = {
                    .send_cq        = rdma_ctx->send_cq,
                    .recv_cq        = rdma_ctx->recv_cq,
                    .srq            = rdma_ctx->srq,
                    .cap            = {
                            .max_send_wr  = DEFAULT_BUF_DEPTH,
                            .max_recv_wr  = DEFAULT_BUF_DEPTH,
                            .max_send_sge = MAX_SEND_SGE,
                            .max_recv_sge = MAX_RECV_SGE
                    },
            };

            switch (config->connection_type) {
                case RC : attr.qp_type = IBV_QPT_RC; break;
                case UC : attr.qp_type = IBV_QPT_UC; break;
                case UD : attr.qp_type = IBV_QPT_UD; break;
                default:  fprintf(stderr, "Unknown connection type \n");
                      return NULL;
            }

    //cm_id在qp之前创建
            ret = rdma_create_qp(rdma_ctx->cm_id, rdma_ctx->pd, &init_attr);
            if (ret != 0) {
                    fs_msg (peer->trans->name, fs_LOG_CRITICAL, errno,
                            RDMA_MSG_CREAT_QP_FAILED, "%s: could not create QP",
                            this->name);
                    ret = -1;
                    goto out;
            }

            rdma_ctx->qp[i] = rdma_ctx->cm_id->qp;
        }


//MR
        int mr_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE ;
        int size = BUF_SIZE;
        for (i = 0; i < config->qp_count; i++) {
            rdma_ctx->buf[i] = memalign(PAGE_SIZE, size);
            if (!rdma_ctx->buf[i])
            {
                fprintf(stderr, "failed to malloc %u bytes to memory buffer\n", size);
                ret = -1;
            }
            memset(rdma_ctx->buf[i], 0, size);
            rdma_ctx->mr[i] = ibv_reg_mr(rdma_ctx->pd, rdma_ctx->buf[i], size, mr_flags);
            if (!rdma_ctx->mr[i]){
                fprintf(stderr, "ibv_reg_mr failed with mr_flags=0x%x\n", mr_flags);
                ret = -1;
                goto out;
            }
        }

out:
        if (ret == -1) {
            for (i = 0; i < config->qp_count; i++) {
                if(rdma_ctx->qp)
                {
                    rdma_destroy_qp(rdma_ctx->cm_id);
                    rdma_ctx->qp = NULL;
                }
                if(rdma_ctx->mr)
                {
                    ibv_dereg_mr(rdma_ctx->mr);
                    rdma_ctx->mr = NULL;
                }
                if(rdma_ctx->buf)
                {
                    free(rdma_ctx->buf);
                    rdma_ctx->buf = NULL;
                }
            }
            if(rdma_ctx->cq)
            {
                ibv_destroy_cq(rdma_ctx->cq);
                rdma_ctx->cq = NULL;
            }
            if(rdma_ctx->pd)
            {
                ibv_dealloc_pd(rdma_ctx->pd);
                rdma_ctx->pd = NULL;
            }
            if(rdma_ctx->content)
            {
                ibv_close_device(rdma_ctx->content);
                rdma_ctx->content = NULL;
            }
            /*
            if(dev_list)
            {
                ibv_free_device_list(dev_list);
                dev_list = NULL;
            }
            */
        }
        return ret;
}

static int fs_rdma_cm_handle_route_resolved (struct rdma_cm_event *event)
{
    /*
struct rdma_conn_param {
    const void *private_data;
    uint8_t private_data_len;
    uint8_t responder_resources;
    uint8_t initiator_depth;
    uint8_t flow_control;
    uint8_t retry_count;        // ignored when accepting
    uint8_t rnr_retry_count;
    // Fields below ignored if a QP is created on the rdma_cm_id.
    uint8_t srq;
    uint32_t qp_num;
};
    */

        struct rdma_conn_param  conn_param = {0, };
        int                     ret        = 0;

        if (event == NULL) {
                goto out;
        }

        ret = fs_rdma_create_resources();
        if (ret != 0) {
                fprintf(stderr, "could not create resources\n");
                fs_rdma_cm_handle_disconnect();
                goto out;
        }

        memset(&conn_param, 0, sizeof conn_param);
        conn_param.responder_resources = 1;
        conn_param.initiator_depth = 1;
        conn_param.retry_count = config->retry_count;
        conn_param.rnr_retry_count = 7; /* infinite retry */

        ret = rdma_connect(rdma_ctx->cm_id, &conn_param);
        if (ret != 0) {
               fprintf(stderr, "rdma_connect failed");
                fs_rdma_cm_handle_disconnect();
                goto out;
        }

        ret = 0;
out:
        return ret;
}

static int
fs_rdma_cm_handle_connect_request (struct rdma_cm_event *event)
{
        int ret = -1;
        ret = fs_rdma_create_resources();
        if (ret != 0) {
                fprintf(stderr, "could not create resources\n");
                fs_rdma_cm_handle_disconnect();
                goto out;
        }

        memset(&conn_param, 0, sizeof conn_param);
        conn_param.responder_resources = 1;
        conn_param.initiator_depth = 1;
        conn_param.retry_count = config->retry_count;
        conn_param.rnr_retry_count = 7; /* infinite retry */

        ret = rdma_accept(child_cm_id, &conn_param);
        if (ret < 0) {
                fprintf(stderr, "rdma_accept failed \n");
                fs_rdma_cm_handle_disconnect ();
                goto out;
        }
        fs_rdma_cm_handle_connect_init (event);
        ret = 0;

out:
        return ret;
}

static void
fs_rdma_cm_handle_disconnect()
{
        fprintf(stdout, "peer disconnected, cleaning up\n");

        fs_rdma_teardown ();
}

static int32_t
fs_rdma_teardown ()
{
    for (i = 0; i < config->qp_count; i++) {
        if(rdma_ctx->qp)
        {
            rdma_destroy_qp(rdma_ctx->cm_id);
            rdma_ctx->qp = NULL;
        }
        if(rdma_ctx->mr)
        {
            ibv_dereg_mr(rdma_ctx->mr);
            rdma_ctx->mr = NULL;
        }
        if(rdma_ctx->buf)
        {
            free(rdma_ctx->buf);
            rdma_ctx->buf = NULL;
        }
    }
    if(rdma_ctx->cq)
    {
        ibv_destroy_cq(rdma_ctx->cq);
        rdma_ctx->cq = NULL;
    }
    if(rdma_ctx->pd)
    {
        ibv_dealloc_pd(rdma_ctx->pd);
        rdma_ctx->pd = NULL;
    }
    if (rdma_ctx->cm_id != NULL) {
            rdma_destroy_id (rdma_ctx->cm_id);
            rdma_ctx->cm_id = NULL;
    }

    /* TODO: decrement cq size */
    return 0;
}

static int
fs_rdma_cm_handle_connect_init (struct rdma_cm_event *event)
{
        cm_id = event->id;

        if (rdma_ctx->connected == 1) {
                fprintf(stderr,  "received event RDMA_CM_EVENT_ESTABLISHED\n");
                ret = -1;
        }
        if (event->id != rdma_ctx->cm_id)
        {
            fprintf(stderr,  "unmatched event\n");
            ret = -1;
        }

        rdma_ctx->connected = 1;

        if (ret < 0) {
                rdma_disconnect (rdma_ctx->cm_id);
        }

        return ret;
}


static int
gf_rdma_cm_handle_event_error ()
{

        fs_rdma_cm_handle_disconnect ();

        return 0;
}

