// MiBench Automotive Benchmark: QuickSort (Adapted C Implementation)
// Source: MiBench Automotive qsort_small / qsort_large

struct Point3D {
    int x;
    int y;
    int z;
    int distance;
};

struct SortMetrics {
    int comparisons;
    int swaps;
    int max_depth_reached;
};

int G_QSORT_THRESHOLD = 7;
int G_MAX_ARRAY_SIZE = 1000;

int point_distance_squared(struct Point3D* p) {
    int dx = p->x;
    int dy = p->y;
    int dz = p->z;
    int d2 = dx * dx + dy * dy + dz * dz;
    return d2;
}

int compare_points(struct Point3D* a, struct Point3D* b) {
    int distA = a->distance;
    int distB = b->distance;
    if (distA < distB) {
        return -1;
    }
    if (distA > distB) {
        return 1;
    }
    return 0;
}

void swap_points(struct Point3D* a, struct Point3D* b) {
    int tx = a->x;
    int ty = a->y;
    int tz = a->z;
    int td = a->distance;

    a->x = b->x;
    a->y = b->y;
    a->z = b->z;
    a->distance = b->distance;

    b->x = tx;
    b->y = ty;
    b->z = tz;
    b->distance = td;
}

void insertion_sort_points(struct Point3D* arr, int low, int high) {
    int i = 0;
    for (i = low + 1; i <= high; i = i + 1) {
        int keyX = arr[i].x;
        int keyY = arr[i].y;
        int keyZ = arr[i].z;
        int keyD = arr[i].distance;
        int j = i - 1;

        while (j >= low) {
            int compVal = 0;
            {
                int currentD = arr[j].distance;
                if (currentD > keyD) {
                    compVal = 1;
                }
            }
            if (compVal == 1) {
                arr[j + 1].x = arr[j].x;
                arr[j + 1].y = arr[j].y;
                arr[j + 1].z = arr[j].z;
                arr[j + 1].distance = arr[j].distance;
                j = j - 1;
            } else {
                j = -1;
            }
        }
        arr[j + 1].x = keyX;
        arr[j + 1].y = keyY;
        arr[j + 1].z = keyZ;
        arr[j + 1].distance = keyD;
    }
}

int partition_points(struct Point3D* arr, int low, int high) {
    int mid = low + (high - low) / 2;
    int pivotDist = arr[mid].distance;

    swap_points(&arr[mid], &arr[high]);

    int i = low;
    int j = 0;
    for (j = low; j < high; j = j + 1) {
        int itemDist = arr[j].distance;
        if (itemDist <= pivotDist) {
            {
                swap_points(&arr[i], &arr[j]);
                i = i + 1;
            }
        }
    }
    swap_points(&arr[i], &arr[high]);
    return i;
}

void quicksort_recursive(struct Point3D* arr, int low, int high, int current_depth) {
    while (low < high) {
        int range = high - low;
        if (range < G_QSORT_THRESHOLD) {
            insertion_sort_points(arr, low, high);
            return;
        }

        {
            int pivotIndex = partition_points(arr, low, high);
            int next_depth = current_depth + 1;

            int leftSpan = pivotIndex - low;
            int rightSpan = high - pivotIndex;

            if (leftSpan < rightSpan) {
                {
                    int subHigh = pivotIndex - 1;
                    quicksort_recursive(arr, low, subHigh, next_depth);
                    low = pivotIndex + 1;
                }
            } else {
                {
                    int subLow = pivotIndex + 1;
                    quicksort_recursive(arr, subLow, high, next_depth);
                    high = pivotIndex - 1;
                }
            }
        }
    }
}

int mibench_run_benchmark(int sample_count) {
    int status = 0;
    struct Point3D points[64];

    int idx = 0;
    for (idx = 0; idx < sample_count; idx = idx + 1) {
        {
            int seed = idx * 17 + 31;
            points[idx].x = seed % 100;
            points[idx].y = (seed * 3) % 100;
            points[idx].z = (seed * 7) % 100;
            points[idx].distance = point_distance_squared(&points[idx]);
        }
    }

    quicksort_recursive(points, 0, sample_count - 1, 0);

    {
        int verify_idx = 0;
        int is_sorted = 1;
        for (verify_idx = 0; verify_idx < sample_count - 1; verify_idx = verify_idx + 1) {
            if (points[verify_idx].distance > points[verify_idx + 1].distance) {
                is_sorted = 0;
            }
        }
        status = is_sorted;
    }

    return status;
}
