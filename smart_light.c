#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/pio.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"
#include "ws2812.pio.h"  // Inclui o arquivo PIO para controle dos LEDs WS2812

// Credenciais WIFI - Substitua com suas credenciais
#define WIFI_SSID "KASATECH CARVALHO"
#define WIFI_PASSWORD "Ph01felix!"

// Definição dos pinos
#define WS2812_PIN 7                    // Pino da matriz de LEDs
#define NUM_PIXELS 25                   // Número de LEDs na matriz (5x5)
#define BTN_B 6                         // Botão para o BOOTSEL

// Modos de iluminação
#define MODE_OFF 0
#define MODE_COLD 1
#define MODE_WARM 2
#define MODE_YELLOW 3
#define MODE_PARTY 4

// Estado global da lâmpada
typedef struct {
    bool is_on;
    int mode;
    int brightness;
    bool party_mode_active;
    uint32_t last_party_update;
    int party_color_index;
} LampState;

LampState lamp_state = {
    .is_on = false,
    .mode = MODE_COLD,
    .brightness = 50,
    .party_mode_active = false,
    .last_party_update = 0,
    .party_color_index = 0
};

// Cores para o modo festa
const uint8_t party_colors[][3] = {
    {255, 0, 0},     // Vermelho
    {0, 255, 0},     // Verde
    {0, 0, 255},     // Azul
    {255, 255, 0},   // Amarelo
    {255, 0, 255},   // Magenta
    {0, 255, 255},   // Ciano
};
#define NUM_PARTY_COLORS 6

// Instância PIO para controle dos LEDs WS2812
PIO pio = pio0;
int sm = 0;

// Protótipos de funções
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
void process_user_request(char *request);
void update_lamp();
void init_ws2812();
uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);
void put_pixel(uint32_t pixel_grb);
void set_all_pixels(uint8_t r, uint8_t g, uint8_t b);
void update_party_mode();

// Inicializa o PIO para controle dos LEDs WS2812
void init_ws2812() {
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);
}

// Converte RGB para o formato GRB usado pelo WS2812
uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

// Envia um pixel para o PIO
void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

// Define a cor de todos os pixels da matriz
void set_all_pixels(uint8_t r, uint8_t g, uint8_t b) {
    // Ajusta o brilho
    float brightness_factor = lamp_state.brightness / 100.0f;
    uint8_t r_adj = (uint8_t)(r * brightness_factor);
    uint8_t g_adj = (uint8_t)(g * brightness_factor);
    uint8_t b_adj = (uint8_t)(b * brightness_factor);
    
    uint32_t color = urgb_u32(r_adj, g_adj, b_adj);
    for (int i = 0; i < NUM_PIXELS; i++) {
        put_pixel(color);
    }
}

// Atualiza o modo festa com cores alternadas
void update_party_mode() {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // Atualiza a cor a cada 200ms
    if (current_time - lamp_state.last_party_update > 200) {
        lamp_state.party_color_index = (lamp_state.party_color_index + 1) % NUM_PARTY_COLORS;
        lamp_state.last_party_update = current_time;
        
        const uint8_t *color = party_colors[lamp_state.party_color_index];
        set_all_pixels(color[0], color[1], color[2]);
    }
}

// Atualiza o estado da lâmpada com base no modo atual
void update_lamp() {
    if (!lamp_state.is_on) {
        set_all_pixels(0, 0, 0);
        return;
    }
    
    switch (lamp_state.mode) {
        case MODE_COLD:
            set_all_pixels(100, 100, 255); // Luz fria (azulada)
            break;
        case MODE_WARM:
            set_all_pixels(255, 140, 20);  // Luz quente (alaranjada)
            break;
        case MODE_YELLOW:
            set_all_pixels(255, 255, 0);   // Luz amarela intensa
            break;
        case MODE_PARTY:
            // O modo festa é atualizado no loop principal
            break;
        default:
            set_all_pixels(0, 0, 0);
            break;
    }
}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

// Processa as requisições do usuário
void process_user_request(char *request) {
    if (strstr(request, "GET /toggle") != NULL) {
        lamp_state.is_on = !lamp_state.is_on;
        printf("Lâmpada %s\n", lamp_state.is_on ? "LIGADA" : "DESLIGADA");
    }
    else if (strstr(request, "GET /mode") != NULL) {
        // Extrai o valor do modo da URL
        char *mode_str = strstr(request, "value=");
        if (mode_str) {
            int mode = atoi(mode_str + 6);
            if (mode >= MODE_OFF && mode <= MODE_PARTY) {
                lamp_state.mode = mode;
                printf("Modo alterado para: %d\n", mode);
            }
        }
    }
    else if (strstr(request, "GET /brightness") != NULL) {
        // Extrai o valor do brilho da URL
        char *brightness_str = strstr(request, "value=");
        if (brightness_str) {
            int brightness = atoi(brightness_str + 6);
            if (brightness >= 0 && brightness <= 100) {
                lamp_state.brightness = brightness;
                printf("Brilho alterado para: %d%%\n", brightness);
            }
        }
    }
    
    // Atualiza o estado da lâmpada
    update_lamp();
}

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Aloca memória para a requisição
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    // Processa a requisição
    process_user_request(request);
    
    // Determina o estado dos botões de modo
    const char *cold_checked = (lamp_state.mode == MODE_COLD) ? "checked" : "";
    const char *warm_checked = (lamp_state.mode == MODE_WARM) ? "checked" : "";
    const char *yellow_checked = (lamp_state.mode == MODE_YELLOW) ? "checked" : "";
    const char *party_checked = (lamp_state.mode == MODE_PARTY) ? "checked" : "";
    
    // Cor de fundo baseada no modo atual (simplificada)
    const char *bg_color = "#222";
    if (lamp_state.is_on) {
        switch (lamp_state.mode) {
            case MODE_COLD: bg_color = "#ccf"; break;    // Azulado claro
            case MODE_WARM: bg_color = "#fc9"; break;    // Alaranjado claro
            case MODE_YELLOW: bg_color = "#ffc"; break;  // Amarelado claro
            case MODE_PARTY: bg_color = "#fcf"; break;   // Rosa claro
        }
    }
    
    // Constrói a página HTML
    char html[2048]; // Reduzido o tamanho do buffer
    snprintf(html, sizeof(html),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>Lamp</title>"
        "<style>"
        "body{font-family:sans-serif;text-align:center;margin:0;padding:10px;background:%s}"
        "h1{margin:10px 0;font-size:24px}"
        "button{background:#555;color:#fff;border:none;padding:10px;margin:5px;border-radius:4px;width:100%%}"
        "input[type=range]{width:100%%}"
        ".on{background:#4a5}"
        ".off{background:#a45}"
        ".box{background:#fff;max-width:300px;margin:0 auto;padding:10px;border-radius:8px}"
        ".row{margin:15px 0}"
        "</style>"
        "</head>"
        "<body>"
        "<div class=\"box\">"
        "<h1>Lampada Inteligente</h1>"
        
        "<div class=\"row\">"
        "<form action=\"/toggle\">"
        "<button class=\"%s\">%s</button>"
        "</form>"
        "</div>"
        
        "<div class=\"row\">"
        "<form id=\"m\" action=\"/mode\">"
        "<div><input type=\"radio\" name=\"value\" value=\"1\" %s onchange=\"this.form.submit()\"><label>Luz Fria</label></div>"
        "<div><input type=\"radio\" name=\"value\" value=\"2\" %s onchange=\"this.form.submit()\"><label>Luz Quente</label></div>"
        "<div><input type=\"radio\" name=\"value\" value=\"3\" %s onchange=\"this.form.submit()\"><label>Luz Amarela</label></div>"
        "<div><input type=\"radio\" name=\"value\" value=\"4\" %s onchange=\"this.form.submit()\"><label>Modo Festa</label></div>"
        "</form>"
        "</div>"
        
        "<div class=\"row\">"
        "<form action=\"/brightness\">"
        "<div>Brilho: <span id=\"bv\">%d%%</span></div>"
        "<input type=\"range\" min=\"0\" max=\"100\" value=\"%d\" name=\"value\" oninput=\"bv.innerText=this.value+'%%'\" onchange=\"this.form.submit()\">"
        "</form>"
        "</div>"
        
        "</div>"
        "</body>"
        "</html>",
        bg_color,                                                   // Cor de fundo baseada no modo
        lamp_state.is_on ? "on" : "off",                            // Classe do botão
        lamp_state.is_on ? "DESLIGAR" : "LIGAR",                    // Texto do botão
        cold_checked, warm_checked, yellow_checked, party_checked,  // Estado dos botões de modo
        lamp_state.brightness, lamp_state.brightness                // Valor do brilho
    );

    // Envia a resposta
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    // Libera a memória
    free(request);
    pbuf_free(p);

    return ERR_OK;
}


void gpio_irq_handler(uint gpio, uint32_t events)
{
  reset_usb_boot(0, 0);
}

int main()
{
    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    

    stdio_init_all();
    printf("Iniciando Smart Lamp Controller...\n");
    sleep_ms(5000); // Delay para acessar o terminal e ver o endereço

    // Inicializa o PIO para WS2812
    init_ws2812();

    // Inicializa o Wi-Fi
    if (cyw43_arch_init()) {
        printf("Falha ao inicializar Wi-Fi\n");
        return -1;
    }

    // Configura o modo de estação Wi-Fi
    cyw43_arch_enable_sta_mode();

    // Conecta à rede Wi-Fi
    printf("Conectando ao Wi-Fi '%s'...\n", WIFI_SSID);
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000)) {
        printf("Falha ao conectar ao Wi-Fi. Tentando novamente...\n");
        sleep_ms(1000);
    }
    printf("Conectado ao Wi-Fi!\n");

    // Exibe o endereço IP
    if (netif_default) {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Configura o servidor TCP
    struct tcp_pcb *server = tcp_new();
    if (!server) {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    // Associa o servidor à porta 80
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }

    // Coloca o servidor em modo de escuta
    server = tcp_listen(server);
    tcp_accept(server, tcp_server_accept);
    printf("Servidor web iniciado na porta 80\n");

    // Apaga todos os LEDs no início
    set_all_pixels(0, 0, 0);

    // Loop principal
    while (true) {
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        
        // Atualiza o modo festa se estiver ativo
        if (lamp_state.is_on && lamp_state.mode == MODE_PARTY) {
            update_party_mode();
        }
        
        sleep_ms(10); // Pequeno delay para reduzir uso da CPU
    }

    cyw43_arch_deinit();
    return 0;
}