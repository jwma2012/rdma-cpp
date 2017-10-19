#include

extern "C" {

/**
 *函数功能，建立RDMA连接
 *可能会在Thrift的server模块被调用
 **/
int establish_connection(struct perftest_comm *comm)
{
    if (comm->rdma_params->machine == CLIENT) {
        if (rdma_client_connect(comm->rdma_ctx, comm->rdma_params)) {
            fprintf(stderr," Unable to perform rdma_client function\n");
            return 1;
        }
    } else {
        if (rdma_server_connect(comm->rdma_ctx,comm->rdma_params)) {
            fprintf(stderr," Unable to perform rdma_server function\n");
            return 1;
        }
    }

    return 0;
}

/**
 *函数功能：客户端发起连接
 *在establish_connection里被调用
 **/
int rdma_client_connect(struct pingpong_context *ctx,struct perftest_parameters *user_param)
{
    char *service;
    int temp,num_of_retry= NUM_OF_RETRIES;
    struct sockaddr_in sin;
    struct addrinfo *res;
    struct rdma_cm_event *event;
    struct rdma_conn_param conn_param;
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (check_add_port(&service,user_param->port,user_param->servername,&hints,&res)) {
        //set param in *res
        fprintf(stderr, "Problem in resolving basic adress and port\n");
        return FAILURE;
    }

    if (res->ai_family != PF_INET) {
        // PF = Protocol Family
        return FAILURE;
    }

    memcpy(&sin, res->ai_addr, sizeof(sin));
    sin.sin_port = htons((unsigned short)user_param->port);

    while (1) {

        if (num_of_retry == 0) {
            fprintf(stderr, "Received %d times ADDR_ERROR\n",NUM_OF_RETRIES);
            return FAILURE;
        }

        if (rdma_resolve_addr(ctx->cm_id, NULL,(struct sockaddr *)&sin,2000)) {
            fprintf(stderr, "rdma_resolve_addr failed\n");
            return FAILURE;
        }

        if (rdma_get_cm_event(ctx->cm_channel,&event)) {
            fprintf(stderr, "rdma_get_cm_events failed\n");
            return FAILURE;
        }

        if (event->event == RDMA_CM_EVENT_ADDR_ERROR) {
            num_of_retry--;
            rdma_ack_cm_event(event);
            continue;
        }

        if (event->event != RDMA_CM_EVENT_ADDR_RESOLVED) {
            fprintf(stderr, "unexpected CM event %d\n",event->event);
            rdma_ack_cm_event(event);
            return FAILURE;
        }

        rdma_ack_cm_event(event);
        break;
    }

    while (1) {

        if (num_of_retry <= 0) {
            fprintf(stderr, "Received %d times ADDR_ERROR - aborting\n",NUM_OF_RETRIES);
            return FAILURE;
        }

        if (rdma_resolve_route(ctx->cm_id,2000)) {
            fprintf(stderr, "rdma_resolve_route failed\n");
            return FAILURE;
        }

        if (rdma_get_cm_event(ctx->cm_channel,&event)) {
            fprintf(stderr, "rdma_get_cm_events failed\n");
            return FAILURE;
        }

        if (event->event == RDMA_CM_EVENT_ROUTE_ERROR) {
            num_of_retry--;
            rdma_ack_cm_event(event);
            continue;
        }

        if (event->event != RDMA_CM_EVENT_ROUTE_RESOLVED) {
            fprintf(stderr, "unexpected CM event %d\n",event->event);
            rdma_ack_cm_event(event);
            return FAILURE;
        }

        rdma_ack_cm_event(event);
        break;
    }

    ctx->context = ctx->cm_id->verbs;
    temp = user_param->work_rdma_cm;
    user_param->work_rdma_cm = ON;

    if (ctx_init(ctx, user_param)) {
        fprintf(stderr," Unable to create the resources needed by comm struct\n");
        return FAILURE;
    }

    memset(&conn_param, 0, sizeof conn_param);
    if (user_param->verb == READ || user_param->verb == ATOMIC) {
        conn_param.responder_resources = user_param->out_reads;
        conn_param.initiator_depth = user_param->out_reads;
    }
    user_param->work_rdma_cm = temp;
    conn_param.retry_count = user_param->retry_count;
    conn_param.rnr_retry_count = 7;

    if (user_param->work_rdma_cm == OFF) {

        if (post_one_recv_wqe(ctx)) {
            fprintf(stderr, "Couldn't post send \n");
            return 1;
        }
    }

    if (rdma_connect(ctx->cm_id,&conn_param)) {
        fprintf(stderr, "Function rdma_connect failed\n");
        return FAILURE;
    }

    if (rdma_get_cm_event(ctx->cm_channel,&event)) {
        fprintf(stderr, "rdma_get_cm_events failed\n");
        return FAILURE;
    }

    if (event->event != RDMA_CM_EVENT_ESTABLISHED) {
        fprintf(stderr, "Unexpected CM event bl blka %d\n", event->event);
        rdma_ack_cm_event(event);
                return FAILURE;
    }

    if (user_param->connection_type == UD) {

        user_param->rem_ud_qpn  = event->param.ud.qp_num;
        user_param->rem_ud_qkey = event->param.ud.qkey;

        ctx->ah[0] = ibv_create_ah(ctx->pd,&event->param.ud.ah_attr);

        if (!ctx->ah) {
            printf(" Unable to create address handler for UD QP\n");
            return FAILURE;
        }
/*
        if (user_param->tst == LAT || (user_param->tst == BW && user_param->duplex)) {

            if (send_qp_num_for_ah(ctx,user_param)) {
                printf(" Unable to send my QP number\n");
                return FAILURE;
            }
        }
    }
*/
    rdma_ack_cm_event(event);
    return SUCCESS;
}

/**
 *函数功能：客户端发起连接
 *在establish_connection里被调用
 */
int rdma_server_connect(struct pingpong_context *ctx,
        struct perftest_parameters *user_param)
{
    int temp;
    struct addrinfo *res;
    struct rdma_cm_event *event;
    struct rdma_conn_param conn_param;
    struct addrinfo hints;
    char *service;
    struct sockaddr_in sin;

    memset(&hints, 0, sizeof hints);
    hints.ai_flags    = AI_PASSIVE;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (check_add_port(&service,user_param->port,user_param->servername,&hints,&res)) {
        fprintf(stderr, "Problem in resolving basic adress and port\n");
        return FAILURE;
    }

    if (res->ai_family != PF_INET) {
        return FAILURE;
    }
    memcpy(&sin, res->ai_addr, sizeof(sin));
    sin.sin_port = htons((unsigned short)user_param->port);

    if (rdma_bind_addr(ctx->cm_id_control,(struct sockaddr *)&sin)) {
        fprintf(stderr," rdma_bind_addr failed\n");
        return 1;
    }

    if (rdma_listen(ctx->cm_id_control,0)) {
        fprintf(stderr, "rdma_listen failed\n");
        return 1;
    }

    if (rdma_get_cm_event(ctx->cm_channel,&event)) {
        fprintf(stderr, "rdma_get_cm_events failed\n");
        return 1;
    }

    if (event->event != RDMA_CM_EVENT_CONNECT_REQUEST) {
        fprintf(stderr, "bad event waiting for connect request %d\n",event->event);
        return 1;
    }

    ctx->cm_id = (struct rdma_cm_id*)event->id;
    ctx->context = ctx->cm_id->verbs;

    if (user_param->work_rdma_cm == ON)
        alloc_ctx(ctx,user_param);

    temp = user_param->work_rdma_cm;
    user_param->work_rdma_cm = ON;

    if (ctx_init(ctx,user_param)) {
        fprintf(stderr," Unable to create the resources needed by comm struct\n");
        return FAILURE;
    }

    memset(&conn_param, 0, sizeof conn_param);
    if (user_param->verb == READ || user_param->verb == ATOMIC) {
        conn_param.responder_resources = user_param->out_reads;
        conn_param.initiator_depth = user_param->out_reads;
    }
    if (user_param->connection_type == UD)
        conn_param.qp_num = ctx->qp[0]->qp_num;

    conn_param.retry_count = user_param->retry_count;
    conn_param.rnr_retry_count = 7;
    user_param->work_rdma_cm = temp;

    if (user_param->work_rdma_cm == OFF) {

        if (post_one_recv_wqe(ctx)) {
            fprintf(stderr, "Couldn't post send \n");
            return 1;
        }

    } else if (user_param->connection_type == UD) {

        if (user_param->tst == LAT || (user_param->tst == BW && user_param->duplex)) {

            if (post_recv_to_get_ah(ctx)) {
                fprintf(stderr, "Couldn't post send \n");
                return 1;
            }
        }
    }

    if (rdma_accept(ctx->cm_id, &conn_param)) {
        fprintf(stderr, "Function rdma_accept failed\n");
        return 1;
    }

    if (user_param->work_rdma_cm && user_param->connection_type == UD) {

        if (user_param->tst == LAT || (user_param->tst == BW && user_param->duplex)) {
            if (create_ah_from_wc_recv(ctx,user_param)) {
                fprintf(stderr, "Unable to create AH from WC\n");
                return 1;
            }
        }
    }

    rdma_ack_cm_event(event);
    rdma_destroy_id(ctx->cm_id_control);
    freeaddrinfo(res);
    return 0;
}



/**
 *函数功能：结束连接
 *在mian里被调用
 **/
int ctx_close_connection(struct perftest_comm *comm,
        struct pingpong_dest *my_dest,
        struct pingpong_dest *rem_dest)
{
    /*Signal client is finished.*/
    if (ctx_hand_shake(comm,my_dest,rem_dest)) {
        return 1;
    }

    if (!comm->rdma_params->use_rdma_cm && !comm->rdma_params->work_rdma_cm) {

        if (write(comm->rdma_params->sockfd,"done",sizeof "done") != sizeof "done") {
            perror(" Client write");
            fprintf(stderr,"Couldn't write to socket\n");
            return -1;
        }

        close(comm->rdma_params->sockfd);
        return 0;
    }

    return 0;
}

/**
 *函数功能：在有限次数内尝试重连
 *在main里被调用
 **/
int retry_rdma_connect(struct pingpong_context *ctx,
        struct perftest_parameters *user_param)
{
    int i, max_retries = 10;
    int delay = 100000; /* 100 millisec */

    for (i = 0; i < max_retries; i++) {
        if (create_rdma_resources(ctx,user_param)) {
            fprintf(stderr," Unable to create rdma resources\n");
            return FAILURE;
        }
        if (rdma_client_connect(ctx,user_param) == SUCCESS)
            return SUCCESS;
        if (destroy_rdma_resources(ctx,user_param)) {
            fprintf(stderr,"Unable to destroy rdma resources\n");
            return FAILURE;
        }
        usleep(delay);
    }
    fprintf(stderr,"Unable to connect (retries = %d)\n", max_retries);
    return FAILURE;
}