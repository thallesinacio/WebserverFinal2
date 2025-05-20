/**
 * AULA IoT - Embarcatech - Ricardo Prates - 004 - Webserver Raspberry Pi Pico w - wlan
 *
 * Material de suporte
 * 
 * https://www.raspberrypi.com/documentation/pico-sdk/networking.html#group_pico_cyw43_arch_1ga33cca1c95fc0d7512e7fef4a59fd7475 
 */

 // led_control_webserver

#include <stdio.h>               // Biblioteca padrão para entrada e saída
#include <string.h>              // Biblioteca manipular strings
#include <stdlib.h>              // funções para realizar várias operações, incluindo alocação de memória dinâmica (malloc)

#include "pico/stdlib.h"         // Biblioteca da Raspberry Pi Pico para funções padrão (GPIO, temporização, etc.)
#include "hardware/adc.h"        // Biblioteca da Raspberry Pi Pico para manipulação do conversor ADC
#include "pico/cyw43_arch.h"     // Biblioteca para arquitetura Wi-Fi da Pico com CYW43  

#include "lwip/pbuf.h"           // Lightweight IP stack - manipulação de buffers de pacotes de rede
#include "lwip/tcp.h"            // Lightweight IP stack - fornece funções e estruturas para trabalhar com o protocolo TCP
#include "lwip/netif.h"          // Lightweight IP stack - fornece funções e estruturas para trabalhar com interfaces de rede (netif)

#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include <string.h>
#include "hardware/pio.h"
#include "pio_matrix.pio.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"


// Credenciais WIFI - Tome cuidado se publicar no github!
#define WIFI_SSID "CHRISTIAAN NET_2.4"
#define WIFI_PASSWORD "24nu4agq@@"

#define BUZZER_PIN 21
void play_sound();

// Definição dos pinos dos LEDs
#define LED_PIN CYW43_WL_GPIO_LED_PIN   // GPIO do CI CYW43
#define LED_BLUE_PIN 12                 // GPIO12 - LED azul
#define LED_GREEN_PIN 11                // GPIO11 - LED verde
#define LED_RED_PIN 13                  // GPIO13 - LED vermelho

// Matriz de led's
PIO pio;
uint sm;
uint32_t VALOR_LED;
unsigned char R, G, B;
#define NUM_PIXELS 25
#define OUT_PIN 7
double v[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.1, 0.0, 0.0, 0.0, 0.1, 0.0, 0.1, 0.0, 0.1, 0.0, 0.0, 0.0};
double apaga[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
double emote[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.1, 0.0, 0.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.1, 0.0, 0.0, 0.0, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1};

// Display
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
char L1[10] = "OFF", L2[10] = "OFF", L3[10] = "OFF", tv[10] = "OFF";

bool flag_buzzer = false;

// Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
void gpio_led_bitdog(void);

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

// Leitura da temperatura interna
float temp_read(void);

// Tratamento do request do usuário
void user_request(char **request);
static ssd1306_t ssd;
// Função principal
int main()
{
    //Inicializa todos os tipos de bibliotecas stdio padrão presentes que estão ligados ao binário.
    stdio_init_all();

    // Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
    gpio_led_bitdog();

    //Inicializa a arquitetura do cyw43
    while (cyw43_arch_init())
    {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }


    // Setando matriz de leds
   double r = 0.0, b = 0.0 , g = 0.0;
   bool ok;
   ok = set_sys_clock_khz(128000, false);
   pio = pio0;

   uint offset = pio_add_program(pio, &WEBSERVER_NOVO_program);
   uint sm = pio_claim_unused_sm(pio, true);
   WEBSERVER_NOVO_program_init(pio, sm, offset, OUT_PIN);

    // Setando display
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN,GPIO_OUT);

    // GPIO do CI CYW43 em nível baixo
    cyw43_arch_gpio_put(LED_PIN, 0);

    // Ativa o Wi-Fi no modo Station, de modo a que possam ser feitas ligações a outros pontos de acesso Wi-Fi.
    cyw43_arch_enable_sta_mode();

    // Conectar à rede WiFI - fazer um loop até que esteja conectado
    printf("Conectando ao Wi-Fi...\n");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }
    printf("Conectado ao Wi-Fi\n");

    // Caso seja a interface de rede padrão - imprimir o IP do dispositivo.
    if (netif_default)
    {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Configura o servidor TCP - cria novos PCBs TCP. É o primeiro passo para estabelecer uma conexão TCP.
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    //vincula um PCB (Protocol Control Block) TCP a um endereço IP e porta específicos.
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }

    // Coloca um PCB (Protocol Control Block) TCP em modo de escuta, permitindo que ele aceite conexões de entrada.
    server = tcp_listen(server);

    // Define uma função de callback para aceitar conexões TCP de entrada. É um passo importante na configuração de servidores TCP.
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");

    // Inicializa o conversor ADC
    adc_init();
    adc_set_temp_sensor_enabled(true);

    while (true)
    {
        ssd1306_divide_em_4_linhas(&ssd);
        ssd1306_draw_string_escala(&ssd, "Luz 1 - ", 4,  4, 0.9);
        ssd1306_draw_string_escala(&ssd, "Luz 2 - ",  4,  20, 0.9);
        ssd1306_draw_string_escala(&ssd, "Luz 3 - ", 4, 36, 0.9);
        ssd1306_draw_string_escala(&ssd, "TV - ", 4, 52, 0.9);
        ssd1306_draw_string_escala(&ssd, L1, 68,  4, 0.9);
        ssd1306_draw_string_escala(&ssd, L2,  68,  20, 0.9);
        ssd1306_draw_string_escala(&ssd, L3, 68, 36, 0.9);
        ssd1306_draw_string_escala(&ssd, tv, 68, 52, 0.9);
        ssd1306_send_data(&ssd);
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        sleep_ms(50);      // Reduz o uso da CPU
    }

    //Desligar a arquitetura CYW43.
    cyw43_arch_deinit();
    return 0;
}

// -------------------------------------- Funções ---------------------------------

// Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
void gpio_led_bitdog(void){
    // Configuração dos LEDs como saída
    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_put(LED_BLUE_PIN, false);
    
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, false);
    
    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, false);
}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

//rotina para definição da intensidade de cores do led
uint32_t matrix_rgb(double b, double r, double g)
{
  //unsigned char R, G, B;
  R = r * 255;
  G = g * 255;
  B = b * 255;
  return (G << 24) | (R << 16) | (B << 8);
}

// Desenha na matriz de leds em verde
void desenho_pio_green(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b){

    for (int16_t i = 0; i < NUM_PIXELS; i++) {
            valor_led = matrix_rgb(b = 0.0, r=0.0, desenho[24-i]);
            pio_sm_put_blocking(pio, sm, valor_led);
    }
}

// Desenha na matriz de leds em vermelho
void desenho_pio_red(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b){

    for (int16_t i = 0; i < NUM_PIXELS; i++) {
            valor_led = matrix_rgb(b = 0.0, desenho[24-i], g = 0.0);
            pio_sm_put_blocking(pio, sm, valor_led);
    }
}

// Tratamento do request do usuário - digite aqui
void user_request(char **request){

    if (strstr(*request, "GET /luz3_on") != NULL)
    {
        gpio_put(LED_BLUE_PIN, 1);
        strcpy(L3, "ON");
    }
    else if (strstr(*request, "GET /luz3_off") != NULL)
    {
        gpio_put(LED_BLUE_PIN, 0);
        strcpy(L3, "OFF");
    }
    else if (strstr(*request, "GET /luz1_on") != NULL)
    {
        gpio_put(LED_GREEN_PIN, 1);
        strcpy(L1, "ON");
    }
    else if (strstr(*request, "GET /luz1_off") != NULL)
    {
        gpio_put(LED_GREEN_PIN, 0);
        strcpy(L1, "OFF");
    }
    else if (strstr(*request, "GET /luz2_on") != NULL)
    {
        gpio_put(LED_RED_PIN, 1);
        strcpy(L2, "ON");
    }
    else if (strstr(*request, "GET /luz2_off") != NULL)
    {
        gpio_put(LED_RED_PIN, 0);
        strcpy(L2, "OFF");
    }
    else if (strstr(*request, "GET /on") != NULL)
    {
        cyw43_arch_gpio_put(LED_PIN, 1);
    }
    else if (strstr(*request, "GET /off") != NULL)
    {
        cyw43_arch_gpio_put(LED_PIN, 0);
    }
    else if (strstr(*request, "GET /tv_on") != NULL)
    {
    printf("Comando recebido: Ligar TV\n");
    desenho_pio_green(emote, VALOR_LED, pio, sm, R, G, B);
    strcpy(tv, "ON");
    }
    else if (strstr(*request, "GET /tv_off") != NULL)
    {
    printf("Comando recebido: Desligar TV\n");
    desenho_pio_green(apaga, VALOR_LED, pio, sm, R, G, B);
    strcpy(tv, "OFF");
    } else if (strstr(*request, "GET /som_on") != NULL) {
    printf("Ativando som\n");
    play_sound();
    }
};


// Ativação do buzzer
void buzz(int BUZZER, uint16_t freq, uint16_t duration) {
    int period = 1000000 / freq;
    int pulse = period / 2;
    int cycles = freq * duration / 1000;
    for (int j = 0; j < cycles; j++) {
        gpio_put(BUZZER, 1);
        sleep_us(pulse);
        gpio_put(BUZZER, 0);
        sleep_us(pulse);
    }
}

void play_sound () {
    int count = 0;
        while(count <= 3) {
            buzz(BUZZER_PIN, 1000, 1000); // beep curto por 1 seg
            sleep_ms(1000);
            ssd1306_divide_em_4_linhas(&ssd);
            ssd1306_draw_string_escala(&ssd, "Luz 1 - ", 4,  4, 0.9);
            ssd1306_draw_string_escala(&ssd, "Luz 2 - ",  4,  20, 0.9);
            ssd1306_draw_string_escala(&ssd, "Luz 3 - ", 4, 36, 0.9);
            ssd1306_draw_string_escala(&ssd, "TV - ", 4, 52, 0.9);
            ssd1306_draw_string_escala(&ssd, L1, 68,  4, 0.9);
            ssd1306_draw_string_escala(&ssd, L2,  68,  20, 0.9);
            ssd1306_draw_string_escala(&ssd, L3, 68, 36, 0.9);
            ssd1306_draw_string_escala(&ssd, tv, 68, 52, 0.9);
            ssd1306_send_data(&ssd);
            count++;
        }
    count = 0;
}


// Leitura da temperatura interna
float temp_read(void){
    adc_select_input(4);
    uint16_t raw_value = adc_read();
    const float conversion_factor = 3.3f / (1 << 12);
    float temperature = 27.0f - ((raw_value * conversion_factor) - 0.706f) / 0.001721f;
        return temperature;
}

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Alocação do request na memória dinámica
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // Tratamento de request - Controle dos LEDs
    user_request(&request);
    
    // Leitura da temperatura interna
    float temperature = temp_read();

    // Cria a resposta HTML
    char html[1024];

    // Instruções html do webserver
    snprintf(html, sizeof(html), 
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n"
"\r\n"
"<!DOCTYPE html><html><head><title>Controle</title><style>"
"body{font-family:Arial;text-align:center;margin:15px}"
".btn{display:inline-block;padding:8px 12px;margin:4px;border:none;border-radius:4px;color:white;text-decoration:none;font-size:14px}"
".l1{background:#4CAF50}.l2{background:#F44336}.l3{background:#2196F3}.tv{background:#9C27B0}.som{background:#FF9800}.off{background:#607D8B}"
"</style></head><body><h2>Controle Residencial</h2>"

"<div><a href='/luz1_on' class='btn l1'>Luz 1 ON</a>"
"<a href='/luz1_off' class='btn off'>OFF</a></div>"

"<div><a href='/luz2_on' class='btn l2'>Luz 2 ON</a>"
"<a href='/luz2_off' class='btn off'>OFF</a></div>"

"<div><a href='/luz3_on' class='btn l3'>Luz 3 ON</a>"
"<a href='/luz3_off' class='btn off'>OFF</a></div>"

"<div><a href='/tv_on' class='btn tv'>TV ON</a>"
"<a href='/tv_off' class='btn off'>OFF</a></div>"

"<div><a href='/som_on' class='btn som'>Som ON</a></div>"

"<p>Temp: %.2f°C</p>"
"</body></html>",

        temperature);

    // Escreve dados para envio (mas não os envia imediatamente).
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);

    // Envia a mensagem
    tcp_output(tpcb);

    //libera memória alocada dinamicamente
    free(request);
    
    //libera um buffer de pacote (pbuf) que foi alocado anteriormente
    pbuf_free(p);

    return ERR_OK;
}

