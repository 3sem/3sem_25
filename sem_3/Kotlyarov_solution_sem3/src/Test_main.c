#include "Transmission_funcs_test.h"

int main(int argc, char* argv[]) {

    if (argc < 4) {

        fprintf(stderr, "Usage: %s <input_file_1> <input_file_2> <input_file_3> (<output_file>)\n", argv[0]);
        exit(1);
    }

    int ret_val = 0;
    double fifo_times[3] = {}, mq_times[3] = {}, shm_times[3] = {};
    char* output_filename = NULL;
    if(argc >= 5)
        output_filename = argv[4];

    for(int i = 0; i < 3; i++) {

        ret_val = Fifo_transmission(argv[i+1], output_filename, fifo_times + i);
        DEBUG_PRINTF("Fifo_transmission exit code: %d\n", ret_val);

        ret_val = SYS_V_mq_transmissio(argv[i+1], output_filename, mq_times + i);
        DEBUG_PRINTF("SYS_V_mq_transmissio exit code: %d\n", ret_val);

        ret_val = SYS_V_shm_transmission(argv[i+1], output_filename, shm_times + i);
        DEBUG_PRINTF("SYS_V_shm_transmission exit code: %d\n", ret_val);
    }

    generate_gnuplot_script(fifo_times, mq_times, shm_times);

    return 0;
}