#include <infiniband/verbs.h>
#include <stdio.h>
#include <string.h>

/* Compare, print, and exit */
#define CPE(val, msg, err_code) \
        if(val) { fprintf(stderr, msg); fprintf(stderr, " Error %d \n", err_code); \
        exit(err_code);}

void hrd_ibv_devinfo(void) {

        int num_devices = 0, dev_i;
        struct ibv_device **dev_list;
        struct ibv_context *ctx;
        struct ibv_device_attr device_attr;

        printf("HRD: printing IB dev info\n");

        dev_list = ibv_get_device_list(&num_devices);
        CPE(!dev_list, "Failed to get IB devices list", 0);

        for (dev_i = 0; dev_i < num_devices; dev_i++) {
                ctx = ibv_open_device(dev_list[dev_i]);
                CPE(!ctx, "Couldn't get context", 0);

                memset(&device_attr, 0, sizeof(device_attr));
                if (ibv_query_device(ctx, &device_attr)) {
                        printf("Could not query device: %d\n", dev_i);
                        exit(-1);
                }

                printf("IB device %d:\n", dev_i);
                printf("    Name: %s\n", dev_list[dev_i]->name);
                printf("    Device name: %s\n", dev_list[dev_i]->dev_name);
                printf("    GUID: %016llx\n",
                        (unsigned long long) ibv_get_device_guid(dev_list[dev_i]));
                printf("    Node type: %d (-1: UNKNOWN, 1: CA, 4: RNIC)\n",
                        dev_list[dev_i]->node_type);
                printf("    Transport type: %d (-1: UNKNOWN, 0: IB, 1: IWARP)\n",
                        dev_list[dev_i]->transport_type);

                printf("    fw: %s\n", device_attr.fw_ver);
                printf("    max_qp: %d\n", device_attr.max_qp);
                printf("    max_cq: %d\n", device_attr.max_cq);
                printf("    max_mr: %d\n", device_attr.max_mr);
                printf("    max_pd: %d\n", device_attr.max_pd);
                printf("    max_ah: %d\n", device_attr.max_ah);
                printf("    phys_port_cnt: %hu\n", device_attr.phys_port_cnt);
                ibv_close_device(ctx);
                ctx = NULL;
        }
        ibv_free_device_list(dev_list);
        dev_list = NULL;

}

int main(int argc, char *argv[]) {

        hrd_ibv_devinfo();
        return 0;
}