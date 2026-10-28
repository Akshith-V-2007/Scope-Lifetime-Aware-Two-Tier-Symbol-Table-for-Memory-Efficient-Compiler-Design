// MiBench Telecommunication Benchmark: Bit Counting Algorithms
// Source: MiBench Automotive/Telecomm bitcnts.c / bitstr.c

struct BitCounterContext {
    int total_bits;
    int words_processed;
    int algorithm_id;
    int iterations;
};

int G_BIT_MASKS[8];

int bit_count_iterative(int n) {
    int count = 0;
    while (n != 0) {
        {
            count = count + (n & 1);
            n = n / 2;
        }
    }
    return count;
}

int bit_count_sparse(int n) {
    int count = 0;
    while (n != 0) {
        {
            count = count + 1;
            n = n & (n - 1);
        }
    }
    return count;
}

int bit_count_parallel(int n) {
    int m1 = 1431655765; // 0x55555555
    int m2 = 858993459;  // 0x33333333
    int m4 = 252645135;  // 0x0f0f0f0f

    int r0 = n;
    int r1 = 0;
    {
        int part1 = r0 & m1;
        int part2 = (r0 / 2) & m1;
        r1 = part1 + part2;
    }

    int r2 = 0;
    {
        int part3 = r1 & m2;
        int part4 = (r1 / 4) & m2;
        r2 = part3 + part4;
    }

    int r3 = 0;
    {
        int part5 = (r2 + (r2 / 16)) & m4;
        r3 = part5;
    }

    int r4 = r3 + (r3 / 256);
    int r5 = r4 + (r4 / 65536);
    int final_count = r5 & 63;
    return final_count;
}

int bit_count_table_lookup(int n) {
    int byte0 = n & 255;
    int byte1 = (n / 256) & 255;
    int byte2 = (n / 65536) & 255;
    int byte3 = (n / 16777216) & 255;

    int c0 = 0;
    int c1 = 0;
    int c2 = 0;
    int c3 = 0;

    {
        int sub_iter = 0;
        for (sub_iter = 0; sub_iter < 8; sub_iter = sub_iter + 1) {
            c0 = c0 + ((byte0 / (1 * (sub_iter + 1))) & 1);
            c1 = c1 + ((byte1 / (1 * (sub_iter + 1))) & 1);
            c2 = c2 + ((byte2 / (1 * (sub_iter + 1))) & 1);
            c3 = c3 + ((byte3 / (1 * (sub_iter + 1))) & 1);
        }
    }

    int total = c0 + c1 + c2 + c3;
    return total;
}

int mibench_eval_bitstream(int stream_seed, int length) {
    struct BitCounterContext ctx;
    ctx.total_bits = 0;
    ctx.words_processed = 0;
    ctx.algorithm_id = 1;
    ctx.iterations = length;

    int current_val = stream_seed;
    int idx = 0;
    for (idx = 0; idx < length; idx = idx + 1) {
        {
            int next_seed = current_val * 1103515245 + 12345;
            int word = next_seed & 1073741823;
            current_val = next_seed;

            int mode = idx % 3;
            int bits = 0;
            if (mode == 0) {
                {
                    int c_sparse = bit_count_sparse(word);
                    bits = c_sparse;
                }
            } else {
                if (mode == 1) {
                    {
                        int c_par = bit_count_parallel(word);
                        bits = c_par;
                    }
                } else {
                    {
                        int c_iter = bit_count_iterative(word);
                        bits = c_iter;
                    }
                }
            }

            ctx.total_bits = ctx.total_bits + bits;
            ctx.words_processed = ctx.words_processed + 1;
        }
    }

    return ctx.total_bits;
}
