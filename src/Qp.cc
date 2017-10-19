static int32_t
gf_rdma_create_qp (rpc_transport_t *this)
{
        gf_rdma_private_t *priv        = NULL;
        gf_rdma_device_t  *device      = NULL;
        int32_t            ret         = 0;
        gf_rdma_peer_t    *peer        = NULL;
        char              *device_name = NULL;

        priv = this->private;

        peer = &priv->peer;

        device_name = (char *)ibv_get_device_name (peer->cm_id->verbs->device);
        if (device_name == NULL) {
                ret = -1;
                gf_msg (this->name, GF_LOG_WARNING, 0,
                        RDMA_MSG_GET_DEVICE_NAME_FAILED, "cannot get "
                        "device_name");
                goto out;
        }

        device = gf_rdma_get_device (this, peer->cm_id->verbs,
                                     device_name);
        if (device == NULL) {
                ret = -1;
                gf_msg (this->name, GF_LOG_WARNING, 0,
                        RDMA_MSG_GET_DEVICE_FAILED, "cannot get device for "
                        "device %s", device_name);
                goto out;
        }

        if (priv->device == NULL) {
                priv->device = device;
        }

        struct ibv_qp_init_attr init_attr = {
                .send_cq        = device->send_cq,
                .recv_cq        = device->recv_cq,
                .srq            = device->srq,
                .cap            = {
                        .max_send_wr  = peer->send_count,
                        .max_recv_wr  = peer->recv_count,
                        .max_send_sge = 2,
                        .max_recv_sge = 1
                },
                .qp_type = IBV_QPT_RC
        };

        ret = rdma_create_qp(peer->cm_id, device->pd, &init_attr);
        if (ret != 0) {
                gf_msg (peer->trans->name, GF_LOG_CRITICAL, errno,
                        RDMA_MSG_CREAT_QP_FAILED, "%s: could not create QP",
                        this->name);
                ret = -1;
                goto out;
        }

        peer->qp = peer->cm_id->qp;

        ret = gf_rdma_register_peer (device, peer->qp->qp_num, peer);

out:
        if (ret == -1)
                __gf_rdma_destroy_qp (this);

        return ret;
}

