#ifndef RDMA_DEVICE_H_
#define RDMA_DEVICE_H_ 1

Class Device {
public:

    struct ibv_device* Find(const char *ib_dev_name);

privite:
    int num_of_device_;
    struct ibv_device **dev_list_;
    struct ibv_device *ib_dev_;

}

struct ibv_device* Find(const char *ib_dev_name)
{
    dev_list_ = ibv_get_device_list(&num_of_device_);

    if (num_of_device_ <= 0) {
        fprintf(stderr," Did not detect devices \n");
        fprintf(stderr," If device exists, check if driver is up\n");
        return NULL;
    }

    if (!ib_dev_name) {
        ib_dev_ = dev_list[0];
        if (!ib_dev_) {
            fprintf(stderr, "No IB devices found\n");
            exit (1);
        }
    } else {
        for (; (ib_dev_ = *dev_list); ++dev_list)
            if (!strcmp(ibv_get_device_name(ib_dev), ib_dev_name))
                break;
        if (!ib_dev_)
            fprintf(stderr, "IB device %s not found\n", ib_dev_name);
    }
    return ib_dev_;
}

#endif // # RDMA_DEVICE_H_