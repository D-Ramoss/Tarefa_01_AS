// Inclusão de bibliotecas
#include <stdio.h> // para entrada e saída padrão
#include "pico/stdlib.h" // para funções padrão do Raspberry Pi Pico
#include "ws2812.pio.h" // para controle dos LEDs WS2812
#include "hardware/gpio.h" // para controle dos pinos GPIO
#include "hardware/timer.h" // para controle dos timers

// Definições de constantes
#define LED_PIN 7 // pino GPIO conectado ao Data In do primeiro LED WS2812
#define NUM_LEDS 25 // número de LEDs na matriz
#define LED_RGB_RED 13 // pino GPIO conectado ao LED RGB Vermelho
#define BUTTON_A 5 // pino GPIO conectado ao Botão A
#define BUTTON_B 6 // pino GPIO conectado ao Botão B

// Definições de cores
#define COR_BLUE rgb_to_grb(0, 0, 255) // cor azul
#define COR_BLACK rgb_to_grb(0, 0, 0) // cor preta

// Padrões de números para uma matriz 5x5
const uint8_t padrao_de_numeros[10][25] = {
    {1, 1, 1, 1, 1, // 0
     1, 0, 0, 0, 1,
     1, 0, 0, 0, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1},
    {0, 0, 1, 0, 0, // 1
     0, 0, 1, 0, 0,
     0, 0, 1, 0, 0,
     0, 1, 1, 0, 0,
     0, 0, 1, 0, 0},
    {1, 1, 1, 1, 1, // 2
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, // 3
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, // 4
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, // 5
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, // 6
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1},
    {0, 0, 0, 0, 1, // 7
     0, 1, 0, 0, 0,
     0, 0, 1, 0, 0,
     1, 0, 0, 1, 0,
     1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, // 8
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, // 9
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1}
};

// Função para converter cor RGB para GRB
uint32_t rgb_to_grb(uint8_t r, uint8_t g, uint8_t b) {
    return ((g << 16) | (r << 8) | b); // formato GRB usado pelos LEDs WS2812
}

// Função para inicializar o PIO para controle dos LEDs WS2812
void ws2812_init(PIO pio, int sm, uint pin) {
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, pin, 800000, false);
}

// Função para definir a cor de um pixel específico
void set_pixel_color(uint32_t color) {
    for (int i = 0; i < NUM_LEDS; i++) {
        pio_sm_put_blocking(pio0, 0, color << 8);
    }
}

// Função para exibir um número na matriz de LEDs
void show_number(int number) {
    uint32_t color = COR_BLUE;
    for (int i = 0; i < NUM_LEDS; i++) {
        if (padrao_de_numeros[number][i]) {
            pio_sm_put_blocking(pio0, 0, color << 8);
        } else {
            pio_sm_put_blocking(pio0, 0, COR_BLACK << 8);
        }
    }
}

// Função chamada pelo timer para piscar o LED RGB Vermelho
bool piscar_led_repetitivo(struct repeating_timer *t) {
    static bool led_state = false;
    led_state = !led_state; // alterna entre ligado e desligado
    gpio_put(LED_RGB_RED, led_state);
    return true; // mantém o timer rodando
}

int main() {
    // Inicializa comunicação UART
    stdio_init_all();

    // Configura e inicializa o PIO para controle dos LEDs WS2812
    PIO pio = pio0;
    int sm = 0; // state machine 0
    ws2812_init(pio, sm, LED_PIN);

    // Configura os botões
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A); // habilita resistor pull-up interno
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B); // habilita resistor pull-up interno

    // Configura o LED RGB
    gpio_init(LED_RGB_RED);
    gpio_set_dir(LED_RGB_RED, GPIO_OUT);

    int number = 0; // número inicial
    show_number(number); // exibe o número inicial (0)

    // Configura um timer para piscar o LED RGB Vermelho a cada 100ms
    struct repeating_timer timer;
    add_repeating_timer_ms(100, piscar_led_repetitivo, NULL, &timer);

    bool button_a_pressed = false;
    bool button_b_pressed = false;
    uint32_t last_debounce_time_a = 0;
    uint32_t last_debounce_time_b = 0;
    const uint32_t debounce_delay = 50; // tempo de debounce em ms

    while (true) {
        // Verifica se o botão A foi pressionado (incrementa o número)
        if (!gpio_get(BUTTON_A) && !button_a_pressed) { // botão A pressionado (nível baixo)
            if (time_us_64() - last_debounce_time_a > debounce_delay * 1000) {
                number++;
                if (number > 9) {
                    number = 0; // volta para 0 após 9
                }
                show_number(number); // atualiza a matriz de LEDs
                button_a_pressed = true;
                last_debounce_time_a = time_us_64();
            }
        } else if (gpio_get(BUTTON_A) && button_a_pressed) {
            button_a_pressed = false;
        }

        // Verifica se o botão B foi pressionado (decrementa o número)
        if (!gpio_get(BUTTON_B) && !button_b_pressed) { // botão B pressionado (nível baixo)
            if (time_us_64() - last_debounce_time_b > debounce_delay * 1000) {
                number--;
                if (number < 0) {
                    number = 9; // volta para 9 após 0
                }
                show_number(number); // atualiza a matriz de LEDs
                button_b_pressed = true;
                last_debounce_time_b = time_us_64();
            }
        } else if (gpio_get(BUTTON_B) && button_b_pressed) {
            button_b_pressed = false;
        }

        // Aguarda um pouco para evitar leituras excessivas dos botões
        sleep_ms(100);
    }

    return 0;
}