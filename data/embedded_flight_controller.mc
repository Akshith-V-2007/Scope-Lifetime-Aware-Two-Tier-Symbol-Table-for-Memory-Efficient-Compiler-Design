// Embedded UAV Flight Controller & Telemetry Navigation Pipeline
// Realistic embedded firmware workload: structs, sensor filtering, AHRS, PID loops, state machines.

struct Vector3 {
    int x;
    int y;
    int z;
};

struct ImuSample {
    int ax;
    int ay;
    int az;
    int gx;
    int gy;
    int gz;
    int temperature;
    int timestamp_ms;
};

struct PidAxis {
    int kp;
    int ki;
    int kd;
    int integral_limit;
    int prev_error;
    int integral_accum;
};

struct FlightState {
    int armed;
    int mode;
    int battery_mv;
    int altitude_cm;
    int target_alt_cm;
    int failsafe_active;
};

// Global System Configuration and Telemetry Channels
int G_SYSTEM_TICK = 0;
int G_ARMED_FLAG = 0;
int G_FAILSAFE_THRESHOLD_MV = 10400;
int G_MAX_ALTITUDE_LIMIT_CM = 50000;
int G_MOTOR_ESC_OUTPUT[4];

int imu_calibrate_and_filter(int raw_acc_x, int raw_acc_y, int raw_acc_z, int temp_c) {
    int calib_bias_x = 14;
    int calib_bias_y = -8;
    int calib_bias_z = 32;
    int filtered_sum = 0;

    {
        // Scope depth 2: Temperature compensation block
        int temp_delta = temp_c - 25;
        int comp_factor_x = temp_delta * 2;
        int comp_factor_y = temp_delta * 3;
        int comp_factor_z = temp_delta * 1;
        
        calib_bias_x = calib_bias_x + comp_factor_x;
        calib_bias_y = calib_bias_y + comp_factor_y;
        calib_bias_z = calib_bias_z + comp_factor_z;
        
        {
            // Scope depth 3: Noise window and outlier rejection
            int sample_x = raw_acc_x - calib_bias_x;
            int sample_y = raw_acc_y - calib_bias_y;
            int sample_z = raw_acc_z - calib_bias_z;
            int norm_sq = sample_x * sample_x + sample_y * sample_y + sample_z * sample_z;
            
            if (norm_sq > 4000000) {
                // Outlier clamp
                sample_x = sample_x / 2;
                sample_y = sample_y / 2;
                sample_z = sample_z / 2;
            } else {
                int smooth_factor = 7;
                sample_x = (sample_x * smooth_factor) / 8;
                sample_y = (sample_y * smooth_factor) / 8;
                sample_z = (sample_z * smooth_factor) / 8;
            }
            filtered_sum = sample_x + sample_y + sample_z;
        }
    }
    return filtered_sum;
}

int pid_compute_axis(int setpoint, int measured, int dt_ms) {
    int error = setpoint - measured;
    int p_term = 0;
    int i_term = 0;
    int d_term = 0;
    int output = 0;

    {
        // Scope depth 2: Proportional & Integral accumulation
        int gain_p = 120;
        int gain_i = 15;
        int gain_d = 45;
        
        p_term = (error * gain_p) / 100;
        
        {
            // Scope depth 3: Anti-windup clamping and integration
            int raw_integral = error * dt_ms;
            int max_i = 500;
            
            if (raw_integral > max_i) {
                raw_integral = max_i;
            } else {
                if (raw_integral < -500) {
                    raw_integral = -500;
                }
            }
            i_term = (raw_integral * gain_i) / 100;
            
            {
                // Scope depth 4: Derivative bandpass filtering
                int delta_err = error - 0;
                int raw_d = (delta_err * 1000) / dt_ms;
                int d_filter_alpha = 65;
                d_term = (raw_d * gain_d * d_filter_alpha) / 10000;
                
                output = p_term + i_term + d_term;
            }
        }
    }
    return output;
}

int navigation_waypoint_step(int cur_lat, int cur_lon, int cur_alt, int target_alt) {
    int heading_command = 0;
    int climb_command = 0;
    int step_result = 0;

    {
        // Scope depth 2: Horizontal trajectory vector
        int dx = cur_lat - 128400;
        int dy = cur_lon - 776500;
        int dist_sq = dx * dx + dy * dy;

        {
            // Scope depth 3: Vertical ascent/descent rate controller
            int alt_error = target_alt - cur_alt;
            int max_climb_rate = 300;
            
            if (alt_error > max_climb_rate) {
                climb_command = max_climb_rate;
            } else {
                if (alt_error < -300) {
                    climb_command = -300;
                } else {
                    climb_command = alt_error;
                }
            }
            
            {
                // Scope depth 4: Cross-track correction and heading steer
                int steer_gain = 35;
                int lateral_error = dy;
                int steer_angle = (lateral_error * steer_gain) / 100;
                
                if (steer_angle > 45) {
                    steer_angle = 45;
                } else {
                    if (steer_angle < -45) {
                        steer_angle = -45;
                    }
                }
                heading_command = steer_angle;
                step_result = dist_sq + climb_command + heading_command;
            }
        }
    }
    return step_result;
}

int flight_mode_state_machine(int current_mode, int vbat_mv, int link_quality) {
    int next_mode = current_mode;
    int status_code = 0;

    {
        // Scope depth 2: Battery safety verification
        int cell_count = 3;
        int cell_mv = vbat_mv / cell_count;
        int critical_cell_mv = 3450;
        int warning_cell_mv = 3600;

        if (cell_mv < critical_cell_mv) {
            // Immediate Land / Cutoff
            next_mode = 9;
            status_code = -1;
        } else {
            if (cell_mv < warning_cell_mv) {
                // Return to launch (RTL)
                next_mode = 6;
                status_code = 1;
            }
        }

        {
            // Scope depth 3: Radio link loss and heartbeat monitoring
            int link_threshold = 20;
            int timeout_counter = 0;
            
            if (link_quality < link_threshold) {
                timeout_counter = timeout_counter + 1;
                if (timeout_counter > 5) {
                    // Failsafe auto-hover
                    next_mode = 4;
                    status_code = 2;
                }
            } else {
                timeout_counter = 0;
            }

            {
                // Scope depth 4: Mode transition validation
                int is_airborne = 1;
                int alt_lock = 1;
                int gps_lock = 1;
                
                if (next_mode == 6) {
                    if (gps_lock == 0) {
                        // Revert to altitude hold if GPS lost
                        next_mode = 3;
                    }
                }
            }
        }
    }
    return next_mode;
}

int motor_mixer_and_esc_output(int throttle, int roll_cmd, int pitch_cmd, int yaw_cmd) {
    int m0 = 0;
    int m1 = 0;
    int m2 = 0;
    int m3 = 0;

    {
        // Scope depth 2: Quad-X Mixer geometry
        int max_pwm = 2000;
        int min_pwm = 1000;
        
        m0 = throttle + pitch_cmd + roll_cmd - yaw_cmd;
        m1 = throttle - pitch_cmd + roll_cmd + yaw_cmd;
        m2 = throttle - pitch_cmd - roll_cmd - yaw_cmd;
        m3 = throttle + pitch_cmd - roll_cmd + yaw_cmd;

        {
            // Scope depth 3: PWM saturation clamping
            if (m0 > max_pwm) { m0 = max_pwm; }
            if (m0 < min_pwm) { m0 = min_pwm; }
            if (m1 > max_pwm) { m1 = max_pwm; }
            if (m1 < min_pwm) { m1 = min_pwm; }
            if (m2 > max_pwm) { m2 = max_pwm; }
            if (m2 < min_pwm) { m2 = min_pwm; }
            if (m3 > max_pwm) { m3 = max_pwm; }
            if (m3 < min_pwm) { m3 = min_pwm; }

            G_MOTOR_ESC_OUTPUT[0] = m0;
            G_MOTOR_ESC_OUTPUT[1] = m1;
            G_MOTOR_ESC_OUTPUT[2] = m2;
            G_MOTOR_ESC_OUTPUT[3] = m3;
        }
    }
    return m0 + m1 + m2 + m3;
}

int magnetometer_compass_fusion(int raw_mx, int raw_my, int raw_mz, int gyro_z_dps, int dt_ms) {
    int mag_heading = 0;
    int fused_heading = 0;
    int declination_deg = 3;

    {
        // Scope depth 2: Hard-iron and soft-iron offset compensation
        int hard_iron_x = 18;
        int hard_iron_y = -35;
        int hard_iron_z = 2;
        int mx_corr = raw_mx - hard_iron_x;
        int my_corr = raw_my - hard_iron_y;
        int mz_corr = raw_mz - hard_iron_z;

        {
            // Scope depth 3: Tilt compensation and raw azimuth estimation
            int field_strength_sq = mx_corr * mx_corr + my_corr * my_corr + mz_corr * mz_corr;
            if (field_strength_sq > 250000) {
                // Earth field normal range check
                int heading_raw = (my_corr * 180) / (mx_corr + 1);
                mag_heading = heading_raw + declination_deg;
            } else {
                mag_heading = 0;
            }

            {
                // Scope depth 4: Gyro integration and complementary yaw fusion
                int gyro_integrated_delta = (gyro_z_dps * dt_ms) / 1000;
                int alpha_mag = 5;
                int alpha_gyro = 95;
                
                fused_heading = (mag_heading * alpha_mag + (gyro_integrated_delta + mag_heading) * alpha_gyro) / 100;
            }
        }
    }
    return fused_heading;
}

int optical_flow_velocity_pipeline(int raw_flow_x, int raw_flow_y, int height_cm, int body_rate_pitch, int body_rate_roll) {
    int vx_est = 0;
    int vy_est = 0;
    int quality_metric = 85;

    {
        // Scope depth 2: Ground distance scaling
        int focal_length_px = 350;
        int height_m_scaled = height_cm / 10;
        int flow_scale = (height_m_scaled * 100) / focal_length_px;

        {
            // Scope depth 3: Angular rotation compensation
            int rot_comp_x = (body_rate_pitch * focal_length_px) / 1000;
            int rot_comp_y = (body_rate_roll * focal_length_px) / 1000;
            int pure_trans_x = raw_flow_x - rot_comp_x;
            int pure_trans_y = raw_flow_y - rot_comp_y;

            {
                // Scope depth 4: Ground speed estimation and surface texture check
                if (quality_metric > 50) {
                    vx_est = (pure_trans_x * flow_scale) / 100;
                    vy_est = (pure_trans_y * flow_scale) / 100;
                } else {
                    vx_est = 0;
                    vy_est = 0;
                }
            }
        }
    }
    return vx_est + vy_est;
}

int telemetry_packet_serialize(int sys_id, int comp_id, int vbat_mv, int alt_cm, int armed) {
    int header_magic = 254;
    int payload_len = 16;
    int seq_id = G_SYSTEM_TICK;
    int crc_accum = 65535;

    {
        // Scope depth 2: Packet framing and payload packing
        int b0 = sys_id;
        int b1 = comp_id;
        int b2 = seq_id;
        int b3 = armed;

        {
            // Scope depth 3: High-frequency telemetry payload
            int vbat_lsb = vbat_mv - (vbat_mv / 256) * 256;
            int vbat_msb = vbat_mv / 256;
            int alt_lsb = alt_cm - (alt_cm / 256) * 256;
            int alt_msb = alt_cm / 256;

            {
                // Scope depth 4: X.25 Checksum accumulation
                int sum0 = b0 + b1 + b2 + b3;
                int sum1 = vbat_lsb + vbat_msb + alt_lsb + alt_msb;
                crc_accum = (crc_accum + sum0 + sum1 + header_magic + payload_len) / 2;
            }
        }
    }
    return crc_accum;
}

int main() {
    int loop_counter = 0;
    int sys_health = 100;
    int raw_ax = 102;
    int raw_ay = -45;
    int raw_az = 988;
    int raw_temp = 31;
    int altitude = 1520;
    int target_alt = 2000;
    int battery = 11800;
    int rc_link = 88;

    while (loop_counter < 10) {
        // Main Flight Executive Loop (Scope Depth 2)
        int acc_filtered = imu_calibrate_and_filter(raw_ax, raw_ay, raw_az, raw_temp);
        int mode = flight_mode_state_machine(1, battery, rc_link);
        
        {
            // Inner Guidance & Control Block (Scope Depth 3)
            int nav_action = navigation_waypoint_step(128420, 776510, altitude, target_alt);
            int roll_correction = pid_compute_axis(0, raw_ax, 10);
            int pitch_correction = pid_compute_axis(0, raw_ay, 10);
            int yaw_correction = pid_compute_axis(0, 0, 10);

            {
                // Inner Mixer & ESC Command Block (Scope Depth 4)
                int base_throttle = 1450;
                int total_thrust = motor_mixer_and_esc_output(
                    base_throttle,
                    roll_correction,
                    pitch_correction,
                    yaw_correction
                );
                
                altitude = altitude + (total_thrust / 100) - 50;
                battery = battery - 2;
                sys_health = sys_health + nav_action - acc_filtered;
            }
        }
        loop_counter = loop_counter + 1;
        G_SYSTEM_TICK = G_SYSTEM_TICK + 1;
    }

    return sys_health;
}
