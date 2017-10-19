#include "Transport.h"
#define INVALID_SOCKET_FD -1
namespace rdma{

using namespace std;

/**
 * RDMA Transport implementation.
 *
 */

Transport::Transport(const string& host, int port)
  : host_(host),
    port_(port),
    sock_fd_(INVALID_SOCKET),
    peerPort_(0),
    connTimeout_(0),
    sendTimeout_(0),
    recvTimeout_(0),
    keepAlive_(false),
    lingerOn_(1),
    lingerVal_(0),
    noDelay_(1),
    maxRecvRetries_(5) {



}
int Transport::Init() {
        rdma_create_id()
        return 0;
}

int Transport::Connect() {

}

int Transport::Listen() {

        ret = rdma_bind_addr (peer->cm_id, &sock_union.sa);
        if (ret != 0) {
                gf_msg (this->name, GF_LOG_WARNING, errno,
                        RDMA_MSG_RDMA_BIND_ADDR_FAILED,
                        "rdma_bind_addr failed");
                goto err;
        }

        ret = rdma_listen (peer->cm_id, priv->backlog);

        if (ret != 0) {
                gf_msg (this->name, GF_LOG_WARNING, errno,
                        RDMA_MSG_LISTEN_FAILED,
                        "rdma_listen failed");
                goto err;
        }
}

int Transport::CtxInit() {

}






























static void *
gf_rdma_cm_event_handler (void *data)
{
        struct rdma_cm_event      *event         = NULL;
        int                        ret           = 0;
        rpc_transport_t           *this          = NULL;
        struct rdma_event_channel *event_channel = NULL;

        event_channel = data;

        while (1) {
                ret = rdma_get_cm_event (event_channel, &event);
                if (ret != 0) {
                        gf_msg (GF_RDMA_LOG_NAME, GF_LOG_WARNING, errno,
                                RDMA_MSG_CM_EVENT_FAILED,
                                "rdma_cm_get_event failed");
                        break;
                }

                switch (event->event) {
                case RDMA_CM_EVENT_ADDR_RESOLVED:
                        gf_rdma_cm_handle_addr_resolved (event);
                        break;

                case RDMA_CM_EVENT_ROUTE_RESOLVED:
                        gf_rdma_cm_handle_route_resolved (event);
                        break;

                case RDMA_CM_EVENT_CONNECT_REQUEST:
                        gf_rdma_cm_handle_connect_request (event);
                        break;

                case RDMA_CM_EVENT_ESTABLISHED:
                        gf_rdma_cm_handle_connect_init (event);
                        break;

                case RDMA_CM_EVENT_ADDR_ERROR:
                case RDMA_CM_EVENT_ROUTE_ERROR:
                case RDMA_CM_EVENT_CONNECT_ERROR:
                case RDMA_CM_EVENT_UNREACHABLE:
                case RDMA_CM_EVENT_REJECTED:
                        this = event->id->context;

                        gf_msg (this->name, GF_LOG_WARNING, 0,
                                RDMA_MSG_CM_EVENT_FAILED, "cma event %s, "
                                "error %d (me:%s peer:%s)\n",
                                rdma_event_str(event->event), event->status,
                                this->myinfo.identifier,
                                this->peerinfo.identifier);

                        rdma_ack_cm_event (event);
                        event = NULL;

                        gf_rdma_cm_handle_event_error (this);
                        continue;

                case RDMA_CM_EVENT_DISCONNECTED:
                        this = event->id->context;

                        gf_msg_debug (this->name, 0, "received disconnect "
                                      "(me:%s peer:%s)\n",
                                      this->myinfo.identifier,
                                      this->peerinfo.identifier);

                        rdma_ack_cm_event (event);
                        event = NULL;

                        gf_rdma_cm_handle_disconnect (this);
                        continue;

                case RDMA_CM_EVENT_DEVICE_REMOVAL:
                        gf_msg (GF_RDMA_LOG_NAME, GF_LOG_WARNING, 0,
                                RDMA_MSG_CM_EVENT_FAILED, "device "
                                "removed");
                        gf_rdma_cm_handle_device_removal (event);
                        break;

                default:
                        gf_msg (GF_RDMA_LOG_NAME, GF_LOG_WARNING, 0,
                                RDMA_MSG_CM_EVENT_FAILED,
                                "unhandled event: %s, ignoring",
                                rdma_event_str(event->event));
                        break;
                }

                rdma_ack_cm_event (event);
        }

        return NULL;
}





int32_t
init (rpc_transport_t *this)
{
        gf_rdma_private_t *priv = NULL;
        gf_rdma_ctx_t *rdma_ctx = NULL;
        struct iobuf_pool *iobuf_pool = NULL;

        priv = GF_CALLOC (1, sizeof (*priv), gf_common_mt_rdma_private_t);
        if (!priv)
                return -1;

        this->private = priv;

        if (gf_rdma_init (this)) {
                gf_msg (this->name, GF_LOG_WARNING, 0,
                        RDMA_MSG_INIT_IB_DEVICE_FAILED,
                        "Failed to initialize IB Device");
                this->private = NULL;
                GF_FREE (priv);
                return -1;
        }
        rdma_ctx = this->ctx->ib;
        pthread_mutex_lock (&rdma_ctx->lock);
        {
                if (rdma_ctx != NULL) {
                        if (this->dl_handle && (++(rdma_ctx->dlcount)) == 1) {
                        iobuf_pool = this->ctx->iobuf_pool;
                        iobuf_pool->rdma_registration = gf_rdma_register_arena;
                        iobuf_pool->rdma_deregistration =
                                                      gf_rdma_deregister_arena;
                        gf_rdma_register_iobuf_pool_with_device
                                                (rdma_ctx->device, iobuf_pool);
                        }
                }
        }
        pthread_mutex_unlock (&rdma_ctx->lock);

        return 0;
}

