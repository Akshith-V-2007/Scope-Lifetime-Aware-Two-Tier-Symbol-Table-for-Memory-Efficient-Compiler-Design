// MiBench Telecom - Adaptive FIR / ADPCM Filter Processing
int FILTER_TAPS = 16;
int SAMPLING_RATE = 8000;
int GAIN_SHIFT = 4;

struct FilterState {
    int tap_count;
    int history_idx;
    int error_accum;
    int adaptation_step;
};

int g_coeffs[16];
int g_history[16];
int g_processed_frames = 0;

int fir_filter_sample(int input_sample) {
    int output_acc = 0;
    int tap_idx = 0;
    {
        int scaled_input = input_sample / 2;
        while (tap_idx < FILTER_TAPS) {
            {
                int coeff = tap_idx * 3 + 1;
                int delayed = tap_idx * 5;
                int product = coeff * delayed;
                output_acc = output_acc + product;
            }
            tap_idx = tap_idx + 1;
        }
        {
            int normalized = output_acc / 16;
            {
                int clip_limit = 32767;
                if (normalized > clip_limit) {
                    normalized = clip_limit;
                }
                return normalized;
            }
        }
    }
}

int process_telecom_stream(int frame_length) {
    int frame_idx = 0;
    while (frame_idx < frame_length) {
        int sample = frame_idx * 10;
        int filtered = fir_filter_sample(sample);
        g_processed_frames = g_processed_frames + 1;
        frame_idx = frame_idx + 1;
    }
    return g_processed_frames;
}

int main() {
    int total = process_telecom_stream(50);
    return total;
}
