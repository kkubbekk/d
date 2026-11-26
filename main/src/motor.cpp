
#include "stepper_motor.hpp"

void IRAM_ATTR STEPPER_MOTOR::motor_isr_handler(void *arg)
{
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_NUM_0);

    step_state = !step_state;
    gpio_set_level(MOTOR_STEP, step_state);

    (dir_forward == true) ? total_steps++ : total_steps--;
    // target_steps--;

    if (is_motor_stopped || total_steps == target_steps)
    {
        is_motor_stopped = true;

        timer_set_alarm_value(TIMER_GROUP_0, TIMER_NUM_0, IDLE_CLOCK_ISR_SPEED); // wolne tykanie gdy nie jest potrzbny
    }
    else
    {
        is_motor_stopped = false;
    }
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_NUM_0);
}
bool STEPPER_MOTOR::is_stopped()
{
    return is_motor_stopped;
}

void STEPPER_MOTOR::set_current_pos_as_azimuth(float azimuth_deg)
{
    float steps_per_degree = (STEPS_PER_REVOLUTION * microsteps) / 360.0f;
    portENTER_CRITICAL(&timer_mux);
    total_steps = static_cast<int32_t>(roundf(azimuth_deg * steps_per_degree));
    portEXIT_CRITICAL(&timer_mux);
}

void STEPPER_MOTOR::set_current(std::uint16_t hold_current_percent, std::uint16_t run_current_percent)
{
    if (hold_current_percent > 100)
        hold_current_percent = 100;

    if (run_current_percent > 100)
        run_current_percent = 100;

    tmc_2209.set_current(run_current_percent, hold_current_percent);
}

void STEPPER_MOTOR::start_motor()
{

    gpio_set_level(MOTOR_ENABLE, 0);

    step_state = 0;

    gpio_set_level(MOTOR_STEP, step_state);

    timer_pause(TIMER_GROUP_0, TIMER_NUM_0);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_NUM_0, 0);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_NUM_0, ISR_STARTING_COOLDOWN);
    timer_set_alarm(TIMER_GROUP_0, TIMER_NUM_0, TIMER_ALARM_EN);
    timer_start(TIMER_GROUP_0, TIMER_NUM_0);
}

void IRAM_ATTR STEPPER_MOTOR::tim_isr_trampoline(void *arg)
{
    STEPPER_MOTOR *motor = static_cast<STEPPER_MOTOR *>(arg);
    motor->motor_isr_handler(arg);
}
void STEPPER_MOTOR::motor_init(std::uint16_t microsteps, std::uint16_t hold_current, std::uint16_t irun_current, std::uint8_t stallguar_act_point)
{
    gpio_config_t gpio_motor_config = {
        .pin_bit_mask = (1ULL << MOTOR_ENABLE) | (1ULL << MOTOR_STEP) | (1ULL << MOTOR_DIR),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};

    gpio_config(&gpio_motor_config);
    gpio_set_level(MOTOR_ENABLE, 0);
    timer_config_t tim_motor_config = {
        .alarm_en = TIMER_ALARM_EN,
        .counter_en = TIMER_START,
        .intr_type = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = TIMER_AUTORELOAD_EN,
        .clk_src = TIMER_SRC_CLK_DEFAULT,
        .divider = 80};

    timer_init(TIMER_GROUP_0, TIMER_NUM_0, &tim_motor_config);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_NUM_0, 0);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_NUM_0, ISR_STARTING_COOLDOWN);
    timer_isr_register(TIMER_GROUP_0, TIMER_NUM_0, &tim_isr_trampoline, this, ESP_INTR_FLAG_IRAM, NULL);
    timer_enable_intr(TIMER_GROUP_0, TIMER_NUM_0);

    tmc_2209.tmc_init();
    set_microsteps(microsteps);
    set_current(hold_current, irun_current);
    start_motor();
}

float STEPPER_MOTOR::get_current_angle()
{
    return total_steps * 360.0f / (STEPS_PER_REVOLUTION * microsteps);
}

float STEPPER_MOTOR::get_normalized_angle()
{
    float raw_angle = get_current_angle();

    float normalized = fmodf(raw_angle, 360.0f);

    if (normalized < 0.0f)
    {
        normalized += 360.0f;
    }
    return normalized;
}

void STEPPER_MOTOR::move_to_angle(float angle_target)
{
    int64_t now_time_us = esp_timer_get_time();

    float dt_sec = (now_time_us - last_pd_time_us) / 1000000.0f;

    // moze warto na ms


    float curr_angle = get_normalized_angle();

    float err_angle = angle_target - curr_angle;

    if (err_angle > 180.0f)
    {
        err_angle -= 360.0f;
    }
    if (err_angle < -180.0f)
    {
        err_angle += 360.0f;
    }

    float steps_per_deg = (STEPS_PER_REVOLUTION * microsteps) / 360.0f;
    int32_t err_steps = static_cast<int32_t>(roundf(err_angle * steps_per_deg));

    if(err_steps == 0 )
    {

        return;
    }

    dir_forward = (err_steps < 0) ? false : true;

    float p_out = std::abs(err_steps) * KP;

    float current_speed = (dt_sec > 0.0001f && dt_sec < 0.5f) ? std::abs((total_steps - last_pd_time_us)/dt_sec) : 0.0f;

    float speed = p_out - KD * current_speed;

    if (speed < 200)
        speed = 200;
    if (speed > 5000)
        speed = 5000;

    target_steps = total_steps + err_steps;

    gpio_set_level(MOTOR_DIR, dir_forward);

    uint32_t delay = static_cast<uint32_t>(ft / speed);

    last_step_count_pd = total_steps;

    last_pd_time_us = esp_timer_get_time();

    is_motor_stopped = true;

    timer_set_counter_value(TIMER_GROUP_0, TIMER_NUM_0, 0);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_NUM_0, delay);
    timer_set_alarm(TIMER_GROUP_0, TIMER_NUM_0, TIMER_ALARM_EN);
}

void STEPPER_MOTOR::move_by_delta(float delta)
{
    move_to_angle(get_normalized_angle() + delta);
}

void STEPPER_MOTOR::stop_motor()
{
    timer_pause(TIMER_GROUP_0, TIMER_NUM_0);

    timer_set_alarm(TIMER_GROUP_0, TIMER_NUM_0, TIMER_ALARM_DIS);

    gpio_set_level(MOTOR_ENABLE, 0);
}

void STEPPER_MOTOR::set_current_pos_as_zero()
{
    portENTER_CRITICAL(&timer_mux);
    total_steps = 0;
    portEXIT_CRITICAL(&timer_mux);
}

void STEPPER_MOTOR::set_microsteps(uint16_t ms)
{

    if (tmc_2209.set_microsteps(ms) != 0)
    {
        portENTER_CRITICAL(&timer_mux);

        microsteps = ms;

        portEXIT_CRITICAL(&timer_mux);

        ESP_LOGI(TAG, "Mikrokroki zmienione na: %d", ms);
    }
}
