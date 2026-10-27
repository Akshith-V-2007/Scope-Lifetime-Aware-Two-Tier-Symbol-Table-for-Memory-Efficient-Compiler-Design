// Synthetically Generated Benchmark Suite
// Parameters: depth=48, symbols_per_scope=4, shadowing_rate=0.10
int g_root_state = 1;
int g_benchmark_epoch = 2026;

int benchmark_workload(int input_arg) {
    int workload_accum = input_arg;
    {
        int var_d1_s0 = 100;
        int var_d1_s1 = 101;
        int var_d1_s2 = 102;
        int var_d1_s3 = 103;
        workload_accum = workload_accum + var_d1_s3;
        workload_accum = workload_accum + var_d1_s3;
        workload_accum = workload_accum + var_d1_s0;
        workload_accum = workload_accum + var_d1_s1;
        {
            int var_d2_s0 = 200;
            int var_d2_s1 = 201;
            int var_d2_s2 = 202;
            int var_d2_s3 = 203;
            workload_accum = workload_accum + var_d1_s3;
            workload_accum = workload_accum + var_d2_s0;
            workload_accum = workload_accum + var_d1_s0;
            workload_accum = workload_accum + var_d2_s1;
            workload_accum = workload_accum + var_d2_s1;
            {
                int var_d3_s0 = 300;
                int var_d3_s1 = 301;
                int var_d3_s2 = 302;
                int var_d3_s3 = 303;
                workload_accum = workload_accum + var_d3_s0;
                workload_accum = workload_accum + var_d1_s0;
                workload_accum = workload_accum + var_d2_s2;
                workload_accum = workload_accum + var_d1_s0;
                workload_accum = workload_accum + var_d1_s0;
                {
                    int var_d4_s0 = 400;
                    int var_d4_s1 = 401;
                    int var_d4_s2 = 402;
                    int var_d4_s3 = 403;
                    workload_accum = workload_accum + var_d1_s1;
                    workload_accum = workload_accum + var_d1_s1;
                    workload_accum = workload_accum + var_d1_s0;
                    workload_accum = workload_accum + var_d4_s2;
                    workload_accum = workload_accum + var_d2_s2;
                    {
                        int var_d5_s0 = 500;
                        int var_d5_s1 = 501;
                        int var_d5_s2 = 502;
                        int var_d5_s3 = 503;
                        workload_accum = workload_accum + var_d3_s3;
                        workload_accum = workload_accum + var_d4_s3;
                        workload_accum = workload_accum + var_d1_s2;
                        workload_accum = workload_accum + var_d3_s1;
                        workload_accum = workload_accum + var_d3_s3;
                        {
                            int var_d6_s0 = 600;
                            int var_d6_s1 = 601;
                            int var_d6_s2 = 602;
                            int var_d6_s3 = 603;
                            workload_accum = workload_accum + var_d6_s1;
                            workload_accum = workload_accum + var_d4_s3;
                            workload_accum = workload_accum + var_d2_s0;
                            workload_accum = workload_accum + var_d2_s0;
                            workload_accum = workload_accum + var_d2_s1;
                            {
                                int var_d7_s0 = 700;
                                int var_d7_s1 = 701;
                                int var_d7_s2 = 702;
                                int var_d2_s2 = 703;
                                workload_accum = workload_accum + var_d2_s0;
                                workload_accum = workload_accum + var_d6_s0;
                                workload_accum = workload_accum + var_d2_s1;
                                workload_accum = workload_accum + var_d4_s3;
                                workload_accum = workload_accum + var_d3_s3;
                                {
                                    int var_d8_s0 = 800;
                                    int var_d8_s1 = 801;
                                    int var_d2_s0 = 802;
                                    int var_d8_s3 = 803;
                                    workload_accum = workload_accum + var_d6_s0;
                                    workload_accum = workload_accum + var_d8_s3;
                                    workload_accum = workload_accum + var_d3_s2;
                                    workload_accum = workload_accum + var_d5_s3;
                                    workload_accum = workload_accum + var_d1_s1;
                                    {
                                        int var_d9_s0 = 900;
                                        int var_d9_s1 = 901;
                                        int var_d9_s2 = 902;
                                        int var_d9_s3 = 903;
                                        workload_accum = workload_accum + var_d7_s2;
                                        workload_accum = workload_accum + var_d2_s1;
                                        workload_accum = workload_accum + var_d7_s0;
                                        workload_accum = workload_accum + var_d7_s1;
                                        workload_accum = workload_accum + var_d6_s0;
                                        {
                                            int var_d10_s0 = 1000;
                                            int var_d10_s1 = 1001;
                                            int var_d10_s2 = 1002;
                                            int var_d10_s3 = 1003;
                                            workload_accum = workload_accum + var_d8_s0;
                                            workload_accum = workload_accum + var_d6_s2;
                                            workload_accum = workload_accum + var_d1_s1;
                                            workload_accum = workload_accum + var_d10_s0;
                                            workload_accum = workload_accum + var_d3_s3;
                                            {
                                                int var_d11_s0 = 1100;
                                                int var_d11_s1 = 1101;
                                                int var_d11_s2 = 1102;
                                                int var_d11_s3 = 1103;
                                                workload_accum = workload_accum + var_d11_s2;
                                                workload_accum = workload_accum + var_d11_s0;
                                                workload_accum = workload_accum + var_d2_s2;
                                                workload_accum = workload_accum + var_d5_s0;
                                                workload_accum = workload_accum + var_d11_s1;
                                                {
                                                    int var_d12_s0 = 1200;
                                                    int var_d12_s1 = 1201;
                                                    int var_d12_s2 = 1202;
                                                    int var_d10_s3 = 1203;
                                                    workload_accum = workload_accum + var_d10_s2;
                                                    workload_accum = workload_accum + var_d2_s1;
                                                    workload_accum = workload_accum + var_d8_s3;
                                                    workload_accum = workload_accum + var_d10_s3;
                                                    workload_accum = workload_accum + var_d1_s2;
                                                    {
                                                        int var_d13_s0 = 1300;
                                                        int var_d13_s1 = 1301;
                                                        int var_d13_s2 = 1302;
                                                        int var_d13_s3 = 1303;
                                                        workload_accum = workload_accum + var_d10_s0;
                                                        workload_accum = workload_accum + var_d11_s0;
                                                        workload_accum = workload_accum + var_d8_s3;
                                                        workload_accum = workload_accum + var_d13_s1;
                                                        workload_accum = workload_accum + var_d6_s2;
                                                        {
                                                            int var_d14_s0 = 1400;
                                                            int var_d14_s1 = 1401;
                                                            int var_d14_s2 = 1402;
                                                            int var_d14_s3 = 1403;
                                                            workload_accum = workload_accum + var_d14_s1;
                                                            workload_accum = workload_accum + var_d7_s2;
                                                            workload_accum = workload_accum + var_d9_s2;
                                                            workload_accum = workload_accum + var_d5_s0;
                                                            workload_accum = workload_accum + var_d1_s1;
                                                            {
                                                                int var_d15_s0 = 1500;
                                                                int var_d15_s1 = 1501;
                                                                int var_d15_s2 = 1502;
                                                                int var_d15_s3 = 1503;
                                                                workload_accum = workload_accum + var_d3_s3;
                                                                workload_accum = workload_accum + var_d11_s3;
                                                                workload_accum = workload_accum + var_d11_s2;
                                                                workload_accum = workload_accum + var_d8_s3;
                                                                workload_accum = workload_accum + var_d10_s2;
                                                                {
                                                                    int var_d16_s0 = 1600;
                                                                    int var_d16_s1 = 1601;
                                                                    int var_d16_s2 = 1602;
                                                                    int var_d16_s3 = 1603;
                                                                    workload_accum = workload_accum + var_d7_s1;
                                                                    workload_accum = workload_accum + var_d1_s3;
                                                                    workload_accum = workload_accum + var_d11_s2;
                                                                    workload_accum = workload_accum + var_d2_s3;
                                                                    workload_accum = workload_accum + var_d9_s3;
                                                                    {
                                                                        int var_d3_s1 = 1700;
                                                                        int var_d17_s1 = 1701;
                                                                        int var_d17_s2 = 1702;
                                                                        int var_d17_s3 = 1703;
                                                                        workload_accum = workload_accum + var_d2_s0;
                                                                        workload_accum = workload_accum + var_d10_s1;
                                                                        workload_accum = workload_accum + var_d15_s0;
                                                                        workload_accum = workload_accum + var_d14_s0;
                                                                        workload_accum = workload_accum + var_d6_s1;
                                                                        {
                                                                            int var_d18_s0 = 1800;
                                                                            int var_d16_s1 = 1801;
                                                                            int var_d18_s2 = 1802;
                                                                            int var_d18_s3 = 1803;
                                                                            workload_accum = workload_accum + var_d3_s1;
                                                                            workload_accum = workload_accum + var_d6_s3;
                                                                            workload_accum = workload_accum + var_d16_s2;
                                                                            workload_accum = workload_accum + var_d13_s2;
                                                                            workload_accum = workload_accum + var_d10_s1;
                                                                            {
                                                                                int var_d19_s0 = 1900;
                                                                                int var_d19_s1 = 1901;
                                                                                int var_d19_s2 = 1902;
                                                                                int var_d19_s3 = 1903;
                                                                                workload_accum = workload_accum + var_d10_s1;
                                                                                workload_accum = workload_accum + var_d5_s3;
                                                                                workload_accum = workload_accum + var_d12_s1;
                                                                                workload_accum = workload_accum + var_d5_s2;
                                                                                workload_accum = workload_accum + var_d13_s2;
                                                                                {
                                                                                    int var_d20_s0 = 2000;
                                                                                    int var_d20_s1 = 2001;
                                                                                    int var_d20_s2 = 2002;
                                                                                    int var_d20_s3 = 2003;
                                                                                    workload_accum = workload_accum + var_d15_s2;
                                                                                    workload_accum = workload_accum + var_d5_s1;
                                                                                    workload_accum = workload_accum + var_d8_s3;
                                                                                    workload_accum = workload_accum + var_d12_s2;
                                                                                    workload_accum = workload_accum + var_d20_s3;
                                                                                    {
                                                                                        int var_d21_s0 = 2100;
                                                                                        int var_d21_s1 = 2101;
                                                                                        int var_d21_s2 = 2102;
                                                                                        int var_d21_s3 = 2103;
                                                                                        workload_accum = workload_accum + var_d18_s3;
                                                                                        workload_accum = workload_accum + var_d14_s2;
                                                                                        workload_accum = workload_accum + var_d19_s2;
                                                                                        workload_accum = workload_accum + var_d6_s3;
                                                                                        workload_accum = workload_accum + var_d10_s3;
                                                                                        {
                                                                                            int var_d22_s0 = 2200;
                                                                                            int var_d22_s1 = 2201;
                                                                                            int var_d3_s2 = 2202;
                                                                                            int var_d22_s3 = 2203;
                                                                                            workload_accum = workload_accum + var_d4_s1;
                                                                                            workload_accum = workload_accum + var_d16_s0;
                                                                                            workload_accum = workload_accum + var_d14_s2;
                                                                                            workload_accum = workload_accum + var_d22_s1;
                                                                                            workload_accum = workload_accum + var_d4_s0;
                                                                                            {
                                                                                                int var_d23_s0 = 2300;
                                                                                                int var_d23_s1 = 2301;
                                                                                                int var_d23_s2 = 2302;
                                                                                                int var_d23_s3 = 2303;
                                                                                                workload_accum = workload_accum + var_d6_s1;
                                                                                                workload_accum = workload_accum + var_d9_s0;
                                                                                                workload_accum = workload_accum + var_d19_s3;
                                                                                                workload_accum = workload_accum + var_d21_s1;
                                                                                                workload_accum = workload_accum + var_d23_s2;
                                                                                                {
                                                                                                    int var_d24_s0 = 2400;
                                                                                                    int var_d24_s1 = 2401;
                                                                                                    int var_d24_s2 = 2402;
                                                                                                    int var_d24_s3 = 2403;
                                                                                                    workload_accum = workload_accum + var_d22_s1;
                                                                                                    workload_accum = workload_accum + var_d15_s3;
                                                                                                    workload_accum = workload_accum + var_d5_s0;
                                                                                                    workload_accum = workload_accum + var_d8_s0;
                                                                                                    workload_accum = workload_accum + var_d7_s0;
                                                                                                    {
                                                                                                        int var_d20_s0 = 2500;
                                                                                                        int var_d3_s2 = 2501;
                                                                                                        int var_d25_s2 = 2502;
                                                                                                        int var_d25_s3 = 2503;
                                                                                                        workload_accum = workload_accum + var_d3_s1;
                                                                                                        workload_accum = workload_accum + var_d13_s3;
                                                                                                        workload_accum = workload_accum + var_d10_s0;
                                                                                                        workload_accum = workload_accum + var_d10_s3;
                                                                                                        workload_accum = workload_accum + var_d25_s3;
                                                                                                        {
                                                                                                            int var_d26_s0 = 2600;
                                                                                                            int var_d26_s1 = 2601;
                                                                                                            int var_d26_s2 = 2602;
                                                                                                            int var_d26_s3 = 2603;
                                                                                                            workload_accum = workload_accum + var_d6_s0;
                                                                                                            workload_accum = workload_accum + var_d21_s0;
                                                                                                            workload_accum = workload_accum + var_d1_s2;
                                                                                                            workload_accum = workload_accum + var_d16_s1;
                                                                                                            workload_accum = workload_accum + var_d20_s3;
                                                                                                            {
                                                                                                                int var_d27_s0 = 2700;
                                                                                                                int var_d27_s1 = 2701;
                                                                                                                int var_d27_s2 = 2702;
                                                                                                                int var_d27_s3 = 2703;
                                                                                                                workload_accum = workload_accum + var_d2_s2;
                                                                                                                workload_accum = workload_accum + var_d25_s2;
                                                                                                                workload_accum = workload_accum + var_d18_s0;
                                                                                                                workload_accum = workload_accum + var_d23_s2;
                                                                                                                workload_accum = workload_accum + var_d18_s2;
                                                                                                                {
                                                                                                                    int var_d28_s0 = 2800;
                                                                                                                    int var_d28_s1 = 2801;
                                                                                                                    int var_d28_s2 = 2802;
                                                                                                                    int var_d28_s3 = 2803;
                                                                                                                    workload_accum = workload_accum + var_d21_s1;
                                                                                                                    workload_accum = workload_accum + var_d18_s3;
                                                                                                                    workload_accum = workload_accum + var_d9_s0;
                                                                                                                    workload_accum = workload_accum + var_d2_s1;
                                                                                                                    workload_accum = workload_accum + var_d17_s2;
                                                                                                                    {
                                                                                                                        int var_d29_s0 = 2900;
                                                                                                                        int var_d29_s1 = 2901;
                                                                                                                        int var_d29_s2 = 2902;
                                                                                                                        int var_d29_s3 = 2903;
                                                                                                                        workload_accum = workload_accum + var_d14_s1;
                                                                                                                        workload_accum = workload_accum + var_d10_s2;
                                                                                                                        workload_accum = workload_accum + var_d20_s2;
                                                                                                                        workload_accum = workload_accum + var_d24_s0;
                                                                                                                        workload_accum = workload_accum + var_d22_s1;
                                                                                                                        {
                                                                                                                            int var_d30_s0 = 3000;
                                                                                                                            int var_d30_s1 = 3001;
                                                                                                                            int var_d6_s2 = 3002;
                                                                                                                            int var_d30_s3 = 3003;
                                                                                                                            workload_accum = workload_accum + var_d24_s3;
                                                                                                                            workload_accum = workload_accum + var_d4_s2;
                                                                                                                            workload_accum = workload_accum + var_d2_s0;
                                                                                                                            workload_accum = workload_accum + var_d5_s3;
                                                                                                                            workload_accum = workload_accum + var_d27_s3;
                                                                                                                            {
                                                                                                                                int var_d31_s0 = 3100;
                                                                                                                                int var_d31_s1 = 3101;
                                                                                                                                int var_d31_s2 = 3102;
                                                                                                                                int var_d31_s3 = 3103;
                                                                                                                                workload_accum = workload_accum + var_d15_s1;
                                                                                                                                workload_accum = workload_accum + var_d2_s0;
                                                                                                                                workload_accum = workload_accum + var_d14_s0;
                                                                                                                                workload_accum = workload_accum + var_d29_s2;
                                                                                                                                workload_accum = workload_accum + var_d18_s3;
                                                                                                                                {
                                                                                                                                    int var_d32_s0 = 3200;
                                                                                                                                    int var_d32_s1 = 3201;
                                                                                                                                    int var_d32_s2 = 3202;
                                                                                                                                    int var_d32_s3 = 3203;
                                                                                                                                    workload_accum = workload_accum + var_d9_s2;
                                                                                                                                    workload_accum = workload_accum + var_d17_s1;
                                                                                                                                    workload_accum = workload_accum + var_d2_s0;
                                                                                                                                    workload_accum = workload_accum + var_d13_s2;
                                                                                                                                    workload_accum = workload_accum + var_d16_s2;
                                                                                                                                    {
                                                                                                                                        int var_d33_s0 = 3300;
                                                                                                                                        int var_d33_s1 = 3301;
                                                                                                                                        int var_d33_s2 = 3302;
                                                                                                                                        int var_d33_s3 = 3303;
                                                                                                                                        workload_accum = workload_accum + var_d31_s2;
                                                                                                                                        workload_accum = workload_accum + var_d19_s1;
                                                                                                                                        workload_accum = workload_accum + var_d3_s0;
                                                                                                                                        workload_accum = workload_accum + var_d13_s3;
                                                                                                                                        workload_accum = workload_accum + var_d17_s3;
                                                                                                                                        {
                                                                                                                                            int var_d34_s0 = 3400;
                                                                                                                                            int var_d34_s1 = 3401;
                                                                                                                                            int var_d34_s2 = 3402;
                                                                                                                                            int var_d34_s3 = 3403;
                                                                                                                                            workload_accum = workload_accum + var_d2_s2;
                                                                                                                                            workload_accum = workload_accum + var_d21_s0;
                                                                                                                                            workload_accum = workload_accum + var_d20_s1;
                                                                                                                                            workload_accum = workload_accum + var_d11_s2;
                                                                                                                                            workload_accum = workload_accum + var_d2_s2;
                                                                                                                                            {
                                                                                                                                                int var_d35_s0 = 3500;
                                                                                                                                                int var_d18_s0 = 3501;
                                                                                                                                                int var_d23_s0 = 3502;
                                                                                                                                                int var_d35_s3 = 3503;
                                                                                                                                                workload_accum = workload_accum + var_d1_s2;
                                                                                                                                                workload_accum = workload_accum + var_d16_s2;
                                                                                                                                                workload_accum = workload_accum + var_d13_s1;
                                                                                                                                                workload_accum = workload_accum + var_d17_s1;
                                                                                                                                                workload_accum = workload_accum + var_d5_s0;
                                                                                                                                                {
                                                                                                                                                    int var_d36_s0 = 3600;
                                                                                                                                                    int var_d36_s1 = 3601;
                                                                                                                                                    int var_d36_s2 = 3602;
                                                                                                                                                    int var_d27_s0 = 3603;
                                                                                                                                                    workload_accum = workload_accum + var_d7_s2;
                                                                                                                                                    workload_accum = workload_accum + var_d30_s1;
                                                                                                                                                    workload_accum = workload_accum + var_d5_s2;
                                                                                                                                                    workload_accum = workload_accum + var_d23_s2;
                                                                                                                                                    workload_accum = workload_accum + var_d36_s1;
                                                                                                                                                    {
                                                                                                                                                        int var_d37_s0 = 3700;
                                                                                                                                                        int var_d37_s1 = 3701;
                                                                                                                                                        int var_d37_s2 = 3702;
                                                                                                                                                        int var_d37_s3 = 3703;
                                                                                                                                                        workload_accum = workload_accum + var_d7_s0;
                                                                                                                                                        workload_accum = workload_accum + var_d13_s0;
                                                                                                                                                        workload_accum = workload_accum + var_d16_s2;
                                                                                                                                                        workload_accum = workload_accum + var_d1_s3;
                                                                                                                                                        workload_accum = workload_accum + var_d20_s1;
                                                                                                                                                        {
                                                                                                                                                            int var_d38_s0 = 3800;
                                                                                                                                                            int var_d38_s1 = 3801;
                                                                                                                                                            int var_d38_s2 = 3802;
                                                                                                                                                            int var_d38_s3 = 3803;
                                                                                                                                                            workload_accum = workload_accum + var_d18_s0;
                                                                                                                                                            workload_accum = workload_accum + var_d1_s3;
                                                                                                                                                            workload_accum = workload_accum + var_d38_s1;
                                                                                                                                                            workload_accum = workload_accum + var_d31_s2;
                                                                                                                                                            workload_accum = workload_accum + var_d28_s1;
                                                                                                                                                            {
                                                                                                                                                                int var_d39_s0 = 3900;
                                                                                                                                                                int var_d39_s1 = 3901;
                                                                                                                                                                int var_d39_s2 = 3902;
                                                                                                                                                                int var_d39_s3 = 3903;
                                                                                                                                                                workload_accum = workload_accum + var_d26_s2;
                                                                                                                                                                workload_accum = workload_accum + var_d24_s3;
                                                                                                                                                                workload_accum = workload_accum + var_d2_s2;
                                                                                                                                                                workload_accum = workload_accum + var_d19_s0;
                                                                                                                                                                workload_accum = workload_accum + var_d7_s1;
                                                                                                                                                                {
                                                                                                                                                                    int var_d36_s0 = 4000;
                                                                                                                                                                    int var_d40_s1 = 4001;
                                                                                                                                                                    int var_d40_s2 = 4002;
                                                                                                                                                                    int var_d40_s3 = 4003;
                                                                                                                                                                    workload_accum = workload_accum + var_d3_s0;
                                                                                                                                                                    workload_accum = workload_accum + var_d19_s0;
                                                                                                                                                                    workload_accum = workload_accum + var_d10_s3;
                                                                                                                                                                    workload_accum = workload_accum + var_d2_s2;
                                                                                                                                                                    workload_accum = workload_accum + var_d33_s2;
                                                                                                                                                                    {
                                                                                                                                                                        int var_d23_s0 = 4100;
                                                                                                                                                                        int var_d41_s1 = 4101;
                                                                                                                                                                        int var_d10_s3 = 4102;
                                                                                                                                                                        int var_d41_s3 = 4103;
                                                                                                                                                                        workload_accum = workload_accum + var_d2_s2;
                                                                                                                                                                        workload_accum = workload_accum + var_d24_s1;
                                                                                                                                                                        workload_accum = workload_accum + var_d11_s2;
                                                                                                                                                                        workload_accum = workload_accum + var_d12_s2;
                                                                                                                                                                        workload_accum = workload_accum + var_d39_s3;
                                                                                                                                                                        {
                                                                                                                                                                            int var_d42_s0 = 4200;
                                                                                                                                                                            int var_d42_s1 = 4201;
                                                                                                                                                                            int var_d42_s2 = 4202;
                                                                                                                                                                            int var_d42_s3 = 4203;
                                                                                                                                                                            workload_accum = workload_accum + var_d27_s1;
                                                                                                                                                                            workload_accum = workload_accum + var_d3_s2;
                                                                                                                                                                            workload_accum = workload_accum + var_d10_s0;
                                                                                                                                                                            workload_accum = workload_accum + var_d22_s1;
                                                                                                                                                                            workload_accum = workload_accum + var_d37_s1;
                                                                                                                                                                            {
                                                                                                                                                                                int var_d43_s0 = 4300;
                                                                                                                                                                                int var_d43_s1 = 4301;
                                                                                                                                                                                int var_d43_s2 = 4302;
                                                                                                                                                                                int var_d43_s3 = 4303;
                                                                                                                                                                                workload_accum = workload_accum + var_d39_s0;
                                                                                                                                                                                workload_accum = workload_accum + var_d19_s1;
                                                                                                                                                                                workload_accum = workload_accum + var_d41_s1;
                                                                                                                                                                                workload_accum = workload_accum + var_d26_s2;
                                                                                                                                                                                workload_accum = workload_accum + var_d8_s1;
                                                                                                                                                                                {
                                                                                                                                                                                    int var_d44_s0 = 4400;
                                                                                                                                                                                    int var_d44_s1 = 4401;
                                                                                                                                                                                    int var_d34_s0 = 4402;
                                                                                                                                                                                    int var_d44_s3 = 4403;
                                                                                                                                                                                    workload_accum = workload_accum + var_d3_s3;
                                                                                                                                                                                    workload_accum = workload_accum + var_d16_s2;
                                                                                                                                                                                    workload_accum = workload_accum + var_d24_s2;
                                                                                                                                                                                    workload_accum = workload_accum + var_d20_s3;
                                                                                                                                                                                    workload_accum = workload_accum + var_d42_s0;
                                                                                                                                                                                    {
                                                                                                                                                                                        int var_d45_s0 = 4500;
                                                                                                                                                                                        int var_d45_s1 = 4501;
                                                                                                                                                                                        int var_d45_s2 = 4502;
                                                                                                                                                                                        int var_d45_s3 = 4503;
                                                                                                                                                                                        workload_accum = workload_accum + var_d13_s3;
                                                                                                                                                                                        workload_accum = workload_accum + var_d37_s3;
                                                                                                                                                                                        workload_accum = workload_accum + var_d44_s0;
                                                                                                                                                                                        workload_accum = workload_accum + var_d31_s1;
                                                                                                                                                                                        workload_accum = workload_accum + var_d21_s0;
                                                                                                                                                                                        {
                                                                                                                                                                                            int var_d46_s0 = 4600;
                                                                                                                                                                                            int var_d46_s1 = 4601;
                                                                                                                                                                                            int var_d46_s2 = 4602;
                                                                                                                                                                                            int var_d46_s3 = 4603;
                                                                                                                                                                                            workload_accum = workload_accum + var_d39_s0;
                                                                                                                                                                                            workload_accum = workload_accum + var_d20_s2;
                                                                                                                                                                                            workload_accum = workload_accum + var_d38_s1;
                                                                                                                                                                                            workload_accum = workload_accum + var_d1_s2;
                                                                                                                                                                                            workload_accum = workload_accum + var_d11_s0;
                                                                                                                                                                                            {
                                                                                                                                                                                                int var_d47_s0 = 4700;
                                                                                                                                                                                                int var_d47_s1 = 4701;
                                                                                                                                                                                                int var_d47_s2 = 4702;
                                                                                                                                                                                                int var_d47_s3 = 4703;
                                                                                                                                                                                                workload_accum = workload_accum + var_d37_s0;
                                                                                                                                                                                                workload_accum = workload_accum + var_d15_s0;
                                                                                                                                                                                                workload_accum = workload_accum + var_d26_s1;
                                                                                                                                                                                                workload_accum = workload_accum + var_d13_s3;
                                                                                                                                                                                                workload_accum = workload_accum + var_d16_s3;
                                                                                                                                                                                                {
                                                                                                                                                                                                    int var_d48_s0 = 4800;
                                                                                                                                                                                                    int var_d48_s1 = 4801;
                                                                                                                                                                                                    int var_d48_s2 = 4802;
                                                                                                                                                                                                    int var_d48_s3 = 4803;
                                                                                                                                                                                                    workload_accum = workload_accum + var_d48_s0;
                                                                                                                                                                                                    workload_accum = workload_accum + var_d6_s3;
                                                                                                                                                                                                    workload_accum = workload_accum + var_d33_s2;
                                                                                                                                                                                                    workload_accum = workload_accum + var_d16_s1;
                                                                                                                                                                                                    workload_accum = workload_accum + var_d24_s1;
                                                                                                                                                                                                }
                                                                                                                                                                                            }
                                                                                                                                                                                        }
                                                                                                                                                                                    }
                                                                                                                                                                                }
                                                                                                                                                                            }
                                                                                                                                                                        }
                                                                                                                                                                    }
                                                                                                                                                                }
                                                                                                                                                            }
                                                                                                                                                        }
                                                                                                                                                    }
                                                                                                                                                }
                                                                                                                                            }
                                                                                                                                        }
                                                                                                                                    }
                                                                                                                                }
                                                                                                                            }
                                                                                                                        }
                                                                                                                    }
                                                                                                                }
                                                                                                            }
                                                                                                        }
                                                                                                    }
                                                                                                }
                                                                                            }
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return workload_accum;
}

int main() {
    int res = benchmark_workload(10);
    return res;
}