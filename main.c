/*José Ribamar Cerqueira Muniz

  ===========================================================================================================
  | Cofre Eletrônico Inteligente: Registro de Acesso e Autenticação Segura com CAPTCHA em Sistema Embarcado |
  ===========================================================================================================

  UTILIZE MOUSE, SEUS BOTÕES OFERECEM MELHOR EXPERIÊNCIA =============================================


  Em resumo, você tem 3 chances para inserir a senha correta no teclado matricial.
  A cada erro vc perde uma chance, quando isso acontece é emitido um sinal pelo buzzer e pelos leds rgb.
  Se vc errar 3 vezes, o cofre é bloqueado por 10 segundos e o buzzer emite som contínuo em sinal
  de alerta. Os 10 segundos são decrementados e mostrados na matriz de leds, os leds rgb fazem animação
  de onda enquando a contagem regressiva ocorre. Após isso o cofre aceita novas entradas de senha.

  Se vc acertar a senha, entrará no modo de verificação, que consistem em utilizar o joystick
  para controlar o led azul e leva-lo até o led vermelho, ambos aparecerão na matriz de leds
  em posições aleatórias. Vc tem que fazer isso pelo menor caminho possível, pois se a posição
  atual do led azul se afastar do vermelho, é acusado como uma tentativa de invasão, então
  o sistema aborta o processo de abertura do cofre e reinicia. Isso também acontece se você
  não completar a verificação dentro de 20 segundos. Se conseguir concluir, terá acesso ao
  cofre por 10 segundos, que são descrementados na matriz de leds, acompanhados por animações
  dos leds rgb e os beeps do buzzer a cada segundo durante esse tempo até que o tempo acabe.
  Então o cofre fecha e o sistema volta ao início. */

//SENHA: 1111, você pode alterar a senha na linha 115

//Inclusão das bibliotecas
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "ws2812b.pio.h"
#include "hardware/adc.h"
#include <stdlib.h>
#include "hardware/gpio.h"
#include "pico/time.h"

// Definições do RTC
#define DS3231_ADDR 0x68
#define DS3231_SEC_REG 0x00
#define DS3231_MIN_REG 0x01
#define DS3231_HOUR_REG 0x02
#define DS3231_DAY_REG 0x03
#define DS3231_DATE_REG 0x04
#define DS3231_MONTH_REG 0x05
#define DS3231_YEAR_REG 0x06

// Definições da matriz de LEDs ws2812
#define NUM_LEDS 35
#define DATA_PIN 16   // porta
#define RED 0xFF0000   // Cor azul
#define BLUE 0x0000FF  // Cor vermelha
#define OFF 0x000000  // LED apagado

// Aqui nós temos as definições dos pinos e portas dos leds rgb
#define LED_GREEN1 2
#define LED_RED1 3
#define LED_BLUE1 1
#define LED_GREEN2 22
#define LED_RED2 28
#define LED_BLUE2 14
#define LED_GREEN3 20
#define LED_RED3 21
#define LED_GREEN4 18
#define LED_RED4 19
#define LED_BLUE3 15

//pino do buzzer
#define BUZZER 17

//pinagem do teclado matricial
#define FILA1 6
#define FILA2 7
#define FILA3 8
#define FILA4 9
#define COL1 10
#define COL2 11
#define COL3 12
#define COL4 13

#define TEMPO_BLOQUEIO 5000

// Aqui nós temos a declaração dos parâmetros do Display LCD
const int LCD_CLEARDISPLAY = 0x01;
const int LCD_RETURNHOME = 0x02;
const int LCD_ENTRYMODESET = 0x04;
const int LCD_DISPLAYCONTROL = 0x08;
const int LCD_CURSORSHIFT = 0x10;
const int LCD_FUNCTIONSET = 0x20;
const int LCD_SETCGRAMADDR = 0x40;
const int LCD_SETDDRAMADDR = 0x80;
const int LCD_ENTRYSHIFTINCREMENT = 0x01;
const int LCD_ENTRYLEFT = 0x02;
const int LCD_BLINKON = 0x01;
const int LCD_CURSORON = 0x02;
const int LCD_DISPLAYON = 0x04;
const int LCD_2LINE = 0x08;
const int LCD_BACKLIGHT = 0x08;
const int LCD_ENABLE_BIT = 0x04;

#define LCD_CHARACTER  1
#define LCD_COMMAND    0
#define MAX_LINES      2
#define MAX_CHARS      16

//pinos do joystick, vert e horz
#define JOYSTICK_X 26  // ADC0
#define JOYSTICK_Y 27  // ADC1

static int lcd_addr = 0x27;

//Definição da senha do sistema
const char senha[5] = "1111";

// configuração da matriz de leds
PIO pio = pio0;
int sm = 0;

// temos a struct do RTC, onde cada campo será responsável pelo seu respectivo dado
typedef struct {
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
  uint8_t day;
  uint8_t date;
  uint8_t month;
  uint8_t year;
} rtc_time_t;

//Aqui é a struct do joystick, cada campo vai armazenar sua posição no seu eixo
typedef struct {
  int x;
  int y;
} Position;

// Aqui são os padros na matriz de leds para representar cada número
const uint8_t numbers[12][7] = {
  {0b00000, 0b01110, 0b01010, 0b01010, 0b01010, 0b01110, 0b00000},  // 0
  {0b00000, 0b00100, 0b01100, 0b00100, 0b00100, 0b01110, 0b00000},  // 1
  {0b00000, 0b01110, 0b00010, 0b01110, 0b01000, 0b01110, 0b00000},  // 2
  {0b00000, 0b01110, 0b00010, 0b01100, 0b00010, 0b01110, 0b00000},  // 3
  {0b00000, 0b01010, 0b01010, 0b01110, 0b00010, 0b01000, 0b00000},  // 4
  {0b00000, 0b01110, 0b01000, 0b01110, 0b00010, 0b01110, 0b00000},  // 5
  {0b00000, 0b01110, 0b01000, 0b01110, 0b01010, 0b01110, 0b00000},  // 6
  {0b00000, 0b01110, 0b00010, 0b01000, 0b00010, 0b01000, 0b00000},  // 7
  {0b00000, 0b01110, 0b01010, 0b01110, 0b01010, 0b01110, 0b00000},  // 8
  {0b00000, 0b01110, 0b01010, 0b01110, 0b00010, 0b01110, 0b00000},  // 9
  {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000},  // 10 Apagado
  {0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111}  // 11 Ligado
};

// Converte um número em formato BCD para decimal para o RTC.
uint8_t bcd_to_dec(uint8_t bcd) {
  return ((bcd >> 4) * 10) + (bcd & 0x0F);
}
//converte numero de decimal para binário, o contrario da função anterior
uint8_t dec_to_bcd(uint8_t dec) {
  return ((dec / 10) << 4) | (dec % 10);
}

//aqui e ainicialização do RTC lendo os valores registrados
void rtc_init() {
  uint8_t reg = DS3231_SEC_REG;
  i2c_write_blocking(i2c_default, DS3231_ADDR, &reg, 1, true);
  uint8_t data;
  i2c_read_blocking(i2c_default, DS3231_ADDR, &data, 1, false);
}
//Define o horário do RTC convertendo os valores para BCD e enviando-os via I2C.
void rtc_set_time(rtc_time_t *time) {
  uint8_t buffer[8];
  buffer[0] = DS3231_SEC_REG;
  buffer[1] = dec_to_bcd(time->seconds);
  buffer[2] = dec_to_bcd(time->minutes);
  buffer[3] = dec_to_bcd(time->hours);
  buffer[4] = dec_to_bcd(time->day);
  buffer[5] = dec_to_bcd(time->date);
  buffer[6] = dec_to_bcd(time->month);
  buffer[7] = dec_to_bcd(time->year);
  i2c_write_blocking(i2c_default, DS3231_ADDR, buffer, 8, false);
}
//Obtém o horário do RTC lendo os valores em BCD e convertendo-os para decimal.
void rtc_get_time(rtc_time_t *time) {
  uint8_t reg = DS3231_SEC_REG;
  i2c_write_blocking(i2c_default, DS3231_ADDR, &reg, 1, true);

  uint8_t buffer[7];
  i2c_read_blocking(i2c_default, DS3231_ADDR, buffer, 7, false);
  time->seconds = bcd_to_dec(buffer[0]);
  time->minutes = bcd_to_dec(buffer[1]);
  time->hours = bcd_to_dec(buffer[2]);
  time->day = bcd_to_dec(buffer[3]);
  time->date = bcd_to_dec(buffer[4]);
  time->month = bcd_to_dec(buffer[5]);
  time->year = bcd_to_dec(buffer[6]);
}

// define as cores dos leds da matriz através do PIO
void set_leds(uint32_t *colors, int num_leds) {
  for (int i = 0; i < num_leds; i++) {
    pio_sm_put_blocking(pio, sm, ((colors[i] >> 8) & 0xFF) | ((colors[i] << 8) & 0xFF00) | (colors[i] & 0xFF0000));
  }
  sleep_ms(10);
}

//exibição dos números na matriz de led
void display_number(int num) {
  uint32_t colors[NUM_LEDS] = {OFF};

  for (int row = 0; row < 7; row++) {
    for (int col = 0; col < 5; col++) {
      int led_index = row * 5 + col;
      if (numbers[num][row] & (1 << (4 - col))) {
        colors[led_index] = BLUE;
      } else {
        colors[led_index] = OFF;
      }
    }
  }

  set_leds(colors, NUM_LEDS);
}

// Enviando byte para o LCD pelo I2C
void i2c_write_byte(uint8_t val) {
  i2c_write_blocking(i2c_default, lcd_addr, &val, 1, false);
}
//ativação do enable do display
void lcd_toggle_enable(uint8_t val) {
  sleep_us(600);
  i2c_write_byte(val | LCD_ENABLE_BIT);
  sleep_us(600);
  i2c_write_byte(val & ~LCD_ENABLE_BIT);
  sleep_us(600);
}

//envia um byte ao lcd um comando ou caractere
void lcd_send_byte(uint8_t val, int mode) {
  uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
  uint8_t low = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;
  i2c_write_byte(high);
  lcd_toggle_enable(high);
  i2c_write_byte(low);
  lcd_toggle_enable(low);
}
//limpando display
void lcd_clear(void) {
  lcd_send_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
}
//posiciona o cursor do lcd na linha especificada
void lcd_set_cursor(int line, int position) {
  int val = (line == 0) ? 0x80 + position : 0xC0 + position;
  lcd_send_byte(val, LCD_COMMAND);
}
//mostra um caractere na tela
void lcd_char(char val) {
  lcd_send_byte(val, LCD_CHARACTER);
}
//exibe uma string no lcd
void lcd_string(const char *s) {
  while (*s) {
    lcd_char(*s++);
  }
}
//inicializa o lcd
void lcd_init() {
  lcd_send_byte(0x03, LCD_COMMAND);
  lcd_send_byte(0x03, LCD_COMMAND);
  lcd_send_byte(0x03, LCD_COMMAND);
  lcd_send_byte(0x02, LCD_COMMAND);

  lcd_send_byte(LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
  lcd_send_byte(LCD_FUNCTIONSET | LCD_2LINE, LCD_COMMAND);
  lcd_send_byte(LCD_DISPLAYCONTROL | LCD_DISPLAYON, LCD_COMMAND);
  lcd_clear();
}

// configuração dos pinos dos leds rgb, teclado matricial, buzzer etc
void configurar_pinos() {
  gpio_init(LED_GREEN1);
  gpio_set_dir(LED_GREEN1, GPIO_OUT);
  gpio_put(LED_GREEN1, 0);
  gpio_init(LED_GREEN2);
  gpio_set_dir(LED_GREEN2, GPIO_OUT);
  gpio_put(LED_GREEN2, 0);
  gpio_init(LED_GREEN3);
  gpio_set_dir(LED_GREEN3, GPIO_OUT);
  gpio_put(LED_GREEN3, 0);
  gpio_init(LED_GREEN4);
  gpio_set_dir(LED_GREEN4, GPIO_OUT);
  gpio_put(LED_GREEN4, 0);
  gpio_init(LED_RED1);
  gpio_set_dir(LED_RED1, GPIO_OUT);
  gpio_put(LED_RED1, 0);
  gpio_init(LED_RED2);
  gpio_set_dir(LED_RED2, GPIO_OUT);
  gpio_put(LED_RED2, 0);
  gpio_init(LED_RED3);
  gpio_set_dir(LED_RED3, GPIO_OUT);
  gpio_put(LED_RED3, 0);
  gpio_init(LED_RED4);
  gpio_set_dir(LED_RED4, GPIO_OUT);
  gpio_put(LED_RED4, 0);
  gpio_init(LED_BLUE1);
  gpio_set_dir(LED_BLUE1, GPIO_OUT);
  gpio_put(LED_BLUE1, 0);
  gpio_init(LED_BLUE2);
  gpio_set_dir(LED_BLUE2, GPIO_OUT);
  gpio_put(LED_BLUE2, 0);
  gpio_init(LED_BLUE3);
  gpio_set_dir(LED_BLUE3, GPIO_OUT);
  gpio_put(LED_BLUE3, 0);

  gpio_init(FILA1); gpio_set_dir(FILA1, GPIO_OUT);
  gpio_init(FILA2); gpio_set_dir(FILA2, GPIO_OUT);
  gpio_init(FILA3); gpio_set_dir(FILA3, GPIO_OUT);
  gpio_init(FILA4); gpio_set_dir(FILA4, GPIO_OUT);

  gpio_init(COL1); gpio_set_dir(COL1, GPIO_IN);
  gpio_init(COL2); gpio_set_dir(COL2, GPIO_IN);
  gpio_init(COL3); gpio_set_dir(COL3, GPIO_IN);
  gpio_init(COL4); gpio_set_dir(COL4, GPIO_IN);

  gpio_init(BUZZER);
  gpio_set_dir(BUZZER, GPIO_OUT);
  gpio_put(BUZZER, 0);
}

//definição dos valores de cada botão do teclado matricial
char ler_teclado() {
  const char teclas[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
  };

  for (int i = 0; i < 4; i++) {
    gpio_put(FILA1, i == 0);
    gpio_put(FILA2, i == 1);
    gpio_put(FILA3, i == 2);
    gpio_put(FILA4, i == 3);

    if (gpio_get(COL1)) return teclas[i][0];
    if (gpio_get(COL2)) return teclas[i][1];
    if (gpio_get(COL3)) return teclas[i][2];
    if (gpio_get(COL4)) return teclas[i][3];
  }
  return 0;
}
/*animação de onda de passo dos leds verdes, para acompanhar o passo
  dos segundos durante a contagem regressiva após a abertura do cofre*/
void onda_verde() {
  // Apaga todos os LEDs verdes primeiro
  gpio_put(LED_GREEN1, 0);
  gpio_put(LED_GREEN2, 0);
  gpio_put(LED_GREEN3, 0);
  gpio_put(LED_GREEN4, 0);
  // Sequência de onda
  const int delay_ms = 125; // Reduced delay for smoother animation
  // Ida
  gpio_put(BUZZER, 1);
  //beep por segundo do buzzer quando o cofre for aberto, isso dura por 10 segundos
  //sendo um beep por segundo

  gpio_put(LED_GREEN1, 1);
  sleep_ms(delay_ms);
  gpio_put(LED_GREEN2, 1);
  sleep_ms(delay_ms);
  gpio_put(LED_GREEN3, 1);
  sleep_ms(delay_ms);
  gpio_put(LED_GREEN4, 1);
  sleep_ms(delay_ms);
  // Volta
  gpio_put(BUZZER, 0);
  gpio_put(LED_GREEN4, 0);
  sleep_ms(delay_ms);
  gpio_put(LED_GREEN3, 0);
  sleep_ms(delay_ms);
  gpio_put(LED_GREEN2, 0);
  sleep_ms(delay_ms);
  gpio_put(LED_GREEN1, 0);
  sleep_ms(delay_ms);
}
//agora na cor vermelha
void onda_vermelha() {
  // Apaga todos os LEDs vermelhos primeiro
  gpio_put(LED_RED1, 0);
  gpio_put(LED_RED2, 0);
  gpio_put(LED_RED3, 0);
  gpio_put(LED_RED4, 0);
  // Sequência de onda
  const int delay_ms = 125;
  // Ida
  gpio_put(LED_RED1, 1);
  sleep_ms(delay_ms);
  gpio_put(LED_RED2, 1);
  sleep_ms(delay_ms);
  gpio_put(LED_RED3, 1);
  sleep_ms(delay_ms);
  gpio_put(LED_RED4, 1);
  sleep_ms(delay_ms);
  // Volta
  gpio_put(LED_RED4, 0);
  sleep_ms(delay_ms);
  gpio_put(LED_RED3, 0);
  sleep_ms(delay_ms);
  gpio_put(LED_RED2, 0);
  sleep_ms(delay_ms);
  gpio_put(LED_RED1, 0);
  sleep_ms(delay_ms);
}
//exibição de numeros na matriz de leds conrrespondentes à apresentação com cor específica
void display_number_with_color(int num, uint32_t color) {
  uint32_t colors[NUM_LEDS] = {OFF};
  for (int row = 0; row < 7; row++) {
    for (int col = 0; col < 5; col++) {
      int led_index = row * 5 + col;
      if (numbers[num][row] & (1 << (4 - col))) {
        colors[led_index] = color;
      } else {
        colors[led_index] = OFF;
      }
    }
  }
  set_leds(colors, NUM_LEDS);
}
//Exibe na matriz de LEDs a quantidade de tentativas restantes,
//representando o número correspondente em cor azul.
void display_attempts_left(int attempts) {
  uint32_t colors[NUM_LEDS] = {OFF};
  const uint8_t (*pattern)[7] = &numbers[3 - attempts];
  for (int row = 0; row < 7; row++) {
    for (int col = 0; col < 5; col++) {
      int led_index = row * 5 + col;
      if ((*pattern)[row] & (1 << (4 - col))) {
        colors[led_index] = BLUE;
      } else {
        colors[led_index] = OFF;
      }
    }
  }

  set_leds(colors, NUM_LEDS);
}
//calculo e exibição inicial da quantidade de tentativas no LCD
void display_attempts_message(int attempts) {
  lcd_clear();
  lcd_string("TEM ");
  lcd_char('0' + attempts);
  lcd_string(" TENTATIVAS");
}

//contagem regressiva após abrir o cofre
void iniciar_contagem_regressiva() {
  lcd_clear();
  lcd_string("  FECHANDO EM");
  lcd_set_cursor(1, 0);
  lcd_string("  10 SEGUNDOS");

  for (int i = 10; i >= 0; i--) {
    display_number_with_color(i, BLUE);  // Numeros na matriz de led em azul
    onda_verde();//função explicada anteriormente com beep por segundo
  }

  lcd_clear();
  lcd_string(" COFRE FECHADO!");
  sleep_ms(1000);
  display_number_with_color(10, BLUE);  //apaga os leds ao fechar o cofre
}

// contagem regressiva de 10 a 0 na matriz de LEDs quando houver bloqueio
void contagem_bloqueio() {
  for (int i = 10; i >= 0; i--) {
    display_number_with_color(i, RED);
    onda_vermelha();
  }
  lcd_clear();
  lcd_string(" COFRE FECHADO!");
  sleep_ms(1000);
  display_number_with_color(10, RED);
}

//registro e exibição da data e hora pelo rtc
void log_incorrect_attempt() {
  rtc_time_t current_time;
  rtc_get_time(&current_time);

  printf("%02d/%02d/20%02d - %02d:%02d:%02d\n\n",
         current_time.date, current_time.month, current_time.year,
         current_time.hours, current_time.minutes, current_time.seconds);
}

//inicialização do joystick no adc
void init_joystick() {
  adc_init();
  adc_gpio_init(JOYSTICK_X);
  adc_gpio_init(JOYSTICK_Y);
}

//gera posição aleatoria na matriz de leds para oo captcha
Position get_random_position() {
  Position pos;
  srand(get_absolute_time());
  pos.x = rand() % 5;
  pos.y = rand() % 7;
  return pos;
}

//acende o led azul e o vermelho em posições diferentes e aleatorias
//na matriz de leds para verificação
void display_verification_leds(Position blue, Position red) {
  uint32_t colors[NUM_LEDS] = {OFF};
  colors[blue.y * 5 + blue.x] = BLUE;
  colors[red.y * 5 + red.x] = RED;
  set_leds(colors, NUM_LEDS);
}

//leitura das coordenadas X e Y do joystick
bool read_joystick(Position* blue_pos) {
  //  X
  adc_select_input(0);
  uint16_t x_raw = adc_read();
  //  Y
  adc_select_input(1);
  uint16_t y_raw = adc_read();

  // Armazena a posição atual antes de modificar
  int old_x = blue_pos->x;
  int old_y = blue_pos->y;

  // Atualiza a posição Y normalmente
  if (y_raw > 3000 && blue_pos->y > 0) blue_pos->y--;
  if (y_raw < 1000 && blue_pos->y < 6) blue_pos->y++;

  // Para o eixo X, inverte a lógica nas linhas ímpares
  if (blue_pos->y % 2 == 0) {
    // Linhas pares (0, 2, 4, 6) - movimento normal
    if (x_raw > 3000 && blue_pos->x > 0) blue_pos->x--;
    if (x_raw < 1000 && blue_pos->x < 4) blue_pos->x++;
  } else {
    // Linhas ímpares (1, 3, 5) - movimento invertido
    if (x_raw > 3000 && blue_pos->x < 4) blue_pos->x++;
    if (x_raw < 1000 && blue_pos->x > 0) blue_pos->x--;
  }
  // Se a posição Y mudou, ajusta a posição X para manter a posição visual correta
  if (old_y != blue_pos->y && old_y % 2 != blue_pos->y % 2) {
    blue_pos->x = 4 - blue_pos->x; // Inverte a posição X
  }
  sleep_ms(200);
  //essa estrutura está assim por conta que a matriz de leds está em
  //zig zag, criando a necessidade de uma adaptação no codigo
  return true;
}

//liga todos os leds da matriz para emitir algum sinal
//e acende os leds rgb na cor correspondente à entrada como parâmetros
void acender(int nume) {
  display_number_with_color(11, nume);
  if (nume == RED) {
    gpio_put(LED_RED1, 1);
    gpio_put(LED_RED2, 1);
    gpio_put(LED_RED3, 1);
    gpio_put(LED_RED4, 1);
  } else {
    gpio_put(LED_GREEN1, 1);
    gpio_put(LED_GREEN2, 1);
    gpio_put(LED_GREEN3, 1);
    gpio_put(LED_GREEN4, 1);
  }
  sleep_ms(2000);
  reset_all_leds();
  display_number_with_color(10, nume);
}

//verificação das posições dos leds azul e vermelho na matriz de leds
//sistema de verificação do CAPTCHA
bool run_verification() {
  Position blue_pos = get_random_position();
  Position red_pos = get_random_position();

  // Certificar que as posições iniciais são diferentes
  while (blue_pos.x == red_pos.x && blue_pos.y == red_pos.y) {
    red_pos = get_random_position();
  }
  //Definir um tempo limite de 20 segundos para a verificação
  absolute_time_t timeout = make_timeout_time_ms(20000);
  //Calcular a distância inicial entre os LEDs
  int last_distance;
  if (blue_pos.y % 2 == 0) {
    if (red_pos.y % 2 == 0) {
      last_distance = abs(blue_pos.x - red_pos.x) + abs(blue_pos.y - red_pos.y);
    } else {
      last_distance = abs(blue_pos.x - (4 - red_pos.x)) + abs(blue_pos.y - red_pos.y);
    }
  } else {
    if (red_pos.y % 2 == 0) {
      last_distance = abs((4 - blue_pos.x) - red_pos.x) + abs(blue_pos.y - red_pos.y);
    } else {
      last_distance = abs((4 - blue_pos.x) - (4 - red_pos.x)) + abs(blue_pos.y - red_pos.y);
    }
  }
  //O loop continua rodando até que a verificação seja bem-sucedida ou o tempo expire
  while (true) {
    int32_t remaining_ms = absolute_time_diff_us(get_absolute_time(), timeout) / 1000;
    if (remaining_ms <= 0) {
      return false;
    }

    // Atualiza display com tempo restante
    lcd_clear();
    lcd_string("Tempo restante:");
    lcd_set_cursor(1, 0);
    char time_str[16];
    snprintf(time_str, sizeof(time_str), "%d segundos", (remaining_ms / 1000) + 1);
    lcd_string(time_str);

    // Ler joystick e atualizar posição do LED azul
    read_joystick(&blue_pos);

    // Exibir ambos os LEDs
    display_verification_leds(blue_pos, red_pos);

    // Verificar distância atual entre os LEDs
    int current_distance;
    if (blue_pos.y % 2 == 0) {
      if (red_pos.y % 2 == 0) {
        current_distance = abs(blue_pos.x - red_pos.x) + abs(blue_pos.y - red_pos.y);
      } else {
        current_distance = abs(blue_pos.x - (4 - red_pos.x)) + abs(blue_pos.y - red_pos.y);
      }
    } else {
      if (red_pos.y % 2 == 0) {
        current_distance = abs((4 - blue_pos.x) - red_pos.x) + abs(blue_pos.y - red_pos.y);
      } else {
        current_distance = abs((4 - blue_pos.x) - (4 - red_pos.x)) + abs(blue_pos.y - red_pos.y);
      }
    }
    //caso haja movimento suspeito, como mover o led azul para longe do vermelho
    if (current_distance > last_distance) {
      printf("\n\n\n\nALERTA: CAPTCHA inválido!\nO LED azul se afastou ESTRANHAMENTE do LED vermelho!\n");
      lcd_clear();
      log_incorrect_attempt();
      lcd_string("CAPTCHA invalido");
      acender(RED);
      return false; // Falha na verificação
    }

    // Atualiza a última distância registrada
    last_distance = current_distance;

    // Se os LEDs coincidirem, a verificação foi bem-sucedida
    if (blue_pos.x == red_pos.x && blue_pos.y == red_pos.y) {
      return true;
    }
  }
  lcd_clear();
}

//desliga todos os leds
void reset_all_leds() {
  gpio_put(LED_RED1, 0);
  gpio_put(LED_RED2, 0);
  gpio_put(LED_RED3, 0);
  gpio_put(LED_RED4, 0);
  gpio_put(LED_GREEN1, 0);
  gpio_put(LED_GREEN2, 0);
  gpio_put(LED_GREEN3, 0);
  gpio_put(LED_GREEN4, 0);
  gpio_put(LED_BLUE1, 0);
  gpio_put(LED_BLUE2, 0);
  gpio_put(LED_BLUE3, 0);
}

int main() {
  //inicialização
  stdio_init_all();
  printf("\n\n=== SISTEMA DE COFRE INICIADO ===\n");
  // Inicializar I2C e LCD
  i2c_init(i2c_default, 100 * 1000);
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
  gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

  // RTC
  rtc_init();
  lcd_init();
  // Inicializar PIO para matriz de LEDs
  uint offset = pio_add_program(pio, &ws2812_program);
  ws2812_program_init(pio, sm, offset, DATA_PIN, 800000, false);
  //configuração dos pinos
  configurar_pinos();

  char senha_digitada[5] = "";
  int tentativas = 3;
  //mostra as tentativas
  display_attempts_message(tentativas);
  while (true) {
    //se as tentativas zerarem, o cofre é bloqueado entrando nesse if
    if (tentativas <= 0) {
      lcd_clear();
      lcd_string("COFRE BLOQUEADO!!!");
      lcd_set_cursor(1, 0);
      lcd_string("STOP 10 SEGUNDOS");
      printf("ATENÇÃO! COFRE BLOQUEADO!!!\n");
      printf("TENTATIVA DE ACESSO INDEVIDO EM: ");
      log_incorrect_attempt();
      //buzzer emite som contínuo durante os 10 segundos de bloqueio sem parar
      //para indicar que houve tentativa de invasão
      gpio_put(BUZZER, 1);
      contagem_bloqueio();
      gpio_put(BUZZER, 0);
      tentativas = 3;  // Reseta as tentativas
      display_attempts_message(tentativas);  // mostra as tentativas restantes
    }
    //recebe a entrada pelo teclado
    char tecla = ler_teclado();
    if (tecla) {
      //Se a senha ainda não tiver 4 caracteres,
      //adiciona a tecla pressionada à string senha_digitada e exibe no LCD
      int len = strlen(senha_digitada);
      if (len < 4) {
        senha_digitada[len] = tecla;
        senha_digitada[len + 1] = '\0';
        lcd_clear();
        lcd_string(senha_digitada);
        //aqui acende um leds rgb para cada dígito inserido na senha
        switch (len) {
          case 0:
            reset_all_leds();
            gpio_put(LED_BLUE1, 1);
            break;
          case 1:
            gpio_put(LED_BLUE2, 1);
            break;
          case 2:
            gpio_put(LED_BLUE3, 1);
            break;
        }
      }
      //Quando a senha atingir 4 caracteres, ela é comparada com a senha correta
      if (strlen(senha_digitada) == 4) {
        if (strcmp(senha_digitada, senha) == 0) {
          sleep_ms(500);
          reset_all_leds(); //desliga os leds
          acender(BLUE);//sinal dos leds verdes com matriz azul
          lcd_clear();
          lcd_string("SENHA CORRETA!");
          lcd_set_cursor(1, 0);
          lcd_string("VERIF. CAPTCHA");
          printf("\n\n\nVERIFICAÇÃO CAPTCHA!!! ");
          log_incorrect_attempt();
          printf("\nUse o analógico para levar o LED BLUE até o LED RED \npelo MENOR CAMINHO!!!\n\n");
          sleep_ms(2000);
          reset_all_leds();
          //inicia o joystick para a verificação
          init_joystick();
          /*chega na função mencionada anteriormente, se as posições dos dois
            leds coincidirem dentro dos 20 segundos PELO MENOR
            caminho, a verificação é concluida*/
          if (run_verification()) {
            lcd_clear();
            lcd_string(" COFRE ABERTO!");
            display_number_with_color(11, BLUE);
            //sinais visuais e sonoro com leds da matriz, rgb e buzzer
            acender(BLUE);
            gpio_put(BUZZER, 1);
            sleep_ms(1000);
            gpio_put(BUZZER, 0);
            //registro do acesso pelo RTC
            printf("\n\n\nATENÇÃO!!!\nCOFRE ACESSADO EM: ");
            log_incorrect_attempt();
            display_number_with_color(10, BLUE);
            iniciar_contagem_regressiva();
            tentativas = 3;
            display_attempts_message(tentativas);
          } else {
            /* se o movimento não for pelo menor caminho, se o usuário
              não conseguir concluir a verificação dentro do tempo, o captcha
              vai apresentar erro*/
            lcd_clear();
            lcd_string("CAPTCHA");
            lcd_set_cursor(1, 0);
            lcd_string("FALHOU!");
            acender(RED);
            lcd_clear();
            lcd_string("COFRE FECHADO!");
            display_number_with_color(10, RED);
            sleep_ms(2000);
            tentativas = 3;
            display_attempts_message(tentativas);
            lcd_clear();
          }
        } else {
          tentativas--;  // Se a senha for incorreta e ainda tiver tentativas, vai decrementar
          lcd_clear();
          reset_all_leds();
          lcd_string("SENHA INCORRETA!");
          lcd_set_cursor(1, 0);
          acender(RED);
          //senha errada, um beep no buzzer
          gpio_put(BUZZER, 1);
          sleep_ms(500);
          gpio_put(BUZZER, 0);
          display_attempts_message(tentativas);
          printf("Tentativa incorreta em: ");

          // Mostra a quantidade de tentativas na matriz de leds
          //dá um pulso com leds vermelho
          display_number_with_color(tentativas, BLUE);
          log_incorrect_attempt(); //registro das horas e data
        }
        display_attempts_message(tentativas);  // mostra as tentativas restantes
        reset_all_leds();
        memset(senha_digitada, 0, sizeof(senha_digitada));
      }
    }
    sleep_ms(200);
  }
  return 0;
}
