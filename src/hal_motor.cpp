#include "hal_motor.h"
#include <driver/ledc.h>
#include <esp_err.h>

namespace {
constexpr int MOTOR_A_PIN = 12;
constexpr int MOTOR_B_PIN = 14;
constexpr uint16_t DUTY_MAX = 1023;

constexpr ledc_mode_t PWM_MODE = LEDC_HIGH_SPEED_MODE;
constexpr ledc_channel_t CHANNEL_A = LEDC_CHANNEL_0;
constexpr ledc_channel_t CHANNEL_B = LEDC_CHANNEL_1;

void configurarCanal(int pin, ledc_channel_t canal) {
    ledc_channel_config_t config = {};
    config.gpio_num = pin;
    config.speed_mode = PWM_MODE;
    config.channel = canal;
    config.intr_type = LEDC_INTR_DISABLE;
    config.timer_sel = LEDC_TIMER_0;
    config.duty = 0;
    config.hpoint = 0;

    ESP_ERROR_CHECK(ledc_channel_config(&config));
}

void escribirPWM(ledc_channel_t canal, uint32_t duty) {
    ESP_ERROR_CHECK(ledc_set_duty(PWM_MODE, canal, duty));
    ESP_ERROR_CHECK(ledc_update_duty(PWM_MODE, canal));
}
}

void motorInit() {
    // Mantiene ambas entradas en LOW durante la configuracion.
    digitalWrite(MOTOR_A_PIN, LOW);
    digitalWrite(MOTOR_B_PIN, LOW);
    pinMode(MOTOR_A_PIN, OUTPUT);
    pinMode(MOTOR_B_PIN, OUTPUT);

    // Primero se configura el temporizador.
    ledc_timer_config_t timer = {};
    timer.speed_mode = PWM_MODE;
    timer.duty_resolution = LEDC_TIMER_10_BIT;
    timer.timer_num = LEDC_TIMER_0;
    timer.freq_hz = 1000;
    timer.clk_cfg = LEDC_AUTO_CLK;

    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    // Despues se configuran los canales, inicialmente a cero.
    configurarCanal(MOTOR_A_PIN, CHANNEL_A);
    configurarCanal(MOTOR_B_PIN, CHANNEL_B);
}

void motorForward(uint16_t duty) {
    if (duty > DUTY_MAX) {
        duty = DUTY_MAX;
    }

    escribirPWM(CHANNEL_B, 0);
    escribirPWM(CHANNEL_A, duty);
}
void motorBrake() {
    // Frenado segun el ejemplo de motores de CRCibernetica.
    escribirPWM(CHANNEL_A, DUTY_MAX);
    escribirPWM(CHANNEL_B, DUTY_MAX);
}

void motorCoast() {
    // Parada libre segun el ejemplo de CRCibernetica.
    escribirPWM(CHANNEL_A, 0);
    escribirPWM(CHANNEL_B, 0);
}