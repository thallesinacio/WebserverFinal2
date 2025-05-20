#include "ssd1306.h"
#include "font.h"

void ssd1306_init(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c) {
  ssd->width = width;
  ssd->height = height;
  ssd->pages = height / 8U;
  ssd->address = address;
  ssd->i2c_port = i2c;
  ssd->bufsize = ssd->pages * ssd->width + 1;
  ssd->ram_buffer = calloc(ssd->bufsize, sizeof(uint8_t));
  ssd->ram_buffer[0] = 0x40;
  ssd->port_buffer[0] = 0x80;
}

void ssd1306_config(ssd1306_t *ssd) {
  ssd1306_command(ssd, SET_DISP | 0x00);
  ssd1306_command(ssd, SET_MEM_ADDR);
  ssd1306_command(ssd, 0x01);
  ssd1306_command(ssd, SET_DISP_START_LINE | 0x00);
  ssd1306_command(ssd, SET_SEG_REMAP | 0x01);
  ssd1306_command(ssd, SET_MUX_RATIO);
  ssd1306_command(ssd, HEIGHT - 1);
  ssd1306_command(ssd, SET_COM_OUT_DIR | 0x08);
  ssd1306_command(ssd, SET_DISP_OFFSET);
  ssd1306_command(ssd, 0x00);
  ssd1306_command(ssd, SET_COM_PIN_CFG);
  ssd1306_command(ssd, 0x12);
  ssd1306_command(ssd, SET_DISP_CLK_DIV);
  ssd1306_command(ssd, 0x80);
  ssd1306_command(ssd, SET_PRECHARGE);
  ssd1306_command(ssd, 0xF1);
  ssd1306_command(ssd, SET_VCOM_DESEL);
  ssd1306_command(ssd, 0x30);
  ssd1306_command(ssd, SET_CONTRAST);
  ssd1306_command(ssd, 0xFF);
  ssd1306_command(ssd, SET_ENTIRE_ON);
  ssd1306_command(ssd, SET_NORM_INV);
  ssd1306_command(ssd, SET_CHARGE_PUMP);
  ssd1306_command(ssd, 0x14);
  ssd1306_command(ssd, SET_DISP | 0x01);
}

void ssd1306_command(ssd1306_t *ssd, uint8_t command) {
  ssd->port_buffer[1] = command;
  i2c_write_blocking(
    ssd->i2c_port,
    ssd->address,
    ssd->port_buffer,
    2,
    false
  );
}

void ssd1306_send_data(ssd1306_t *ssd) {
  ssd1306_command(ssd, SET_COL_ADDR);
  ssd1306_command(ssd, 0);
  ssd1306_command(ssd, ssd->width - 1);
  ssd1306_command(ssd, SET_PAGE_ADDR);
  ssd1306_command(ssd, 0);
  ssd1306_command(ssd, ssd->pages - 1);
  i2c_write_blocking(
    ssd->i2c_port,
    ssd->address,
    ssd->ram_buffer,
    ssd->bufsize,
    false
  );
}

void ssd1306_pixel(ssd1306_t *ssd, uint8_t x, uint8_t y, bool value) {
  uint16_t index = (y >> 3) + (x << 3) + 1;
  uint8_t pixel = (y & 0b111);
  if (value)
    ssd->ram_buffer[index] |= (1 << pixel);
  else
    ssd->ram_buffer[index] &= ~(1 << pixel);
}

/*
void ssd1306_fill(ssd1306_t *ssd, bool value) {
  uint8_t byte = value ? 0xFF : 0x00;
  for (uint8_t i = 1; i < ssd->bufsize; ++i)
    ssd->ram_buffer[i] = byte;
}*/

void ssd1306_fill(ssd1306_t *ssd, bool value) {
    // Itera por todas as posições do display
    for (uint8_t y = 0; y < ssd->height; ++y) {
        for (uint8_t x = 0; x < ssd->width; ++x) {
            ssd1306_pixel(ssd, x, y, value);
        }
    }
}



void ssd1306_rect(ssd1306_t *ssd, uint8_t top, uint8_t left, uint8_t width, uint8_t height, bool value, bool fill) {
  for (uint8_t x = left; x < left + width; ++x) {
    ssd1306_pixel(ssd, x, top, value);
    ssd1306_pixel(ssd, x, top + height - 1, value);
  }
  for (uint8_t y = top; y < top + height; ++y) {
    ssd1306_pixel(ssd, left, y, value);
    ssd1306_pixel(ssd, left + width - 1, y, value);
  }

  if (fill) {
    for (uint8_t x = left + 1; x < left + width - 1; ++x) {
      for (uint8_t y = top + 1; y < top + height - 1; ++y) {
        ssd1306_pixel(ssd, x, y, value);
      }
    }
  }
}

void ssd1306_line(ssd1306_t *ssd, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool value) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int err = dx - dy;

    while (true) {
        ssd1306_pixel(ssd, x0, y0, value); // Desenha o pixel atual

        if (x0 == x1 && y0 == y1) break; // Termina quando alcança o ponto final

        int e2 = err * 2;

        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}


void ssd1306_hline(ssd1306_t *ssd, uint8_t x0, uint8_t x1, uint8_t y, bool value) {
  for (uint8_t x = x0; x <= x1; ++x)
    ssd1306_pixel(ssd, x, y, value);
}

void ssd1306_vline(ssd1306_t *ssd, uint8_t x, uint8_t y0, uint8_t y1, bool value) {
  for (uint8_t y = y0; y <= y1; ++y)
    ssd1306_pixel(ssd, x, y, value);
}

// Função para desenhar um caractere
void ssd1306_draw_char(ssd1306_t *ssd, char c, uint8_t x, uint8_t y)
{
  uint16_t index = 0;
  char ver=c;
  if (c >= 'A' && c <= 'Z')
  {
    index = (c - 'A' + 11) * 8; // Para letras maiúsculas
  }else  if (c >= '0' && c <= '9')
  {
    index = (c - '0' + 1) * 8; // Adiciona o deslocamento necessário
  } else if (c >= 'a' && c <= 'z') 
  {
    index = (c - 'a' + 37) * 8; 
  }
  for (uint8_t i = 0; i < 8; ++i)
  {
    uint8_t line = font2[index + i];
    for (uint8_t j = 0; j < 8; ++j)
    {
      ssd1306_pixel(ssd, x + i, y + j, line & (1 << j));
    }
  }
}

void ssd1306_draw_char_escala(ssd1306_t *ssd, char c, uint8_t x, uint8_t y, float escala)
{
  uint16_t index = 0;

  if (c >= ' ' && c <= '~') {
    index = (c - ' ') * 8;
  }

  for (uint8_t i = 0; i < 8; ++i) {
    uint8_t line = font[index + i];

    for (uint8_t j = 0; j < 8; ++j) {
      if (line & (1 << j)) {
        // Desenha um "bloco" proporcional à escala
        for (uint8_t dx = 0; dx < escala; ++dx) {
          for (uint8_t dy = 0; dy < escala; ++dy) {
            ssd1306_pixel(ssd,
                          x + (uint8_t)(i * escala) + dx,
                          y + (uint8_t)(j * escala) + dy,
                          true);
          }
        }
      }
    }
  }
}

void ssd1306_draw_string_escala(ssd1306_t *ssd, const char *str, uint8_t x, uint8_t y, float escala)
{
  uint8_t char_width = (uint8_t)(8 * escala);
  uint8_t char_height = (uint8_t)(8 * escala);

  while (*str)
  {
    ssd1306_draw_char_escala(ssd, *str++, x, y, escala);
    x += char_width;

    if (x + char_width >= ssd->width) {
      x = 0;
      y += char_height;
    }

    if (y + char_height >= ssd->height) {
      break;
    }
  }
}

void ssd1306_draw_string(ssd1306_t *ssd, const char *str, uint8_t x, uint8_t y)
{
  while (*str)
  {
    ssd1306_draw_char(ssd, *str++, x, y);
    x += 8;
    if (x + 8 >= ssd->width)
    {
      x = 0;
      y += 8;
    }
    if (y + 8 >= ssd->height)
    {
      break;
    }
  }
}

void ssd1306_draw_square(ssd1306_t *ssd, uint8_t x, uint8_t y) {
  // Vamos desenhar um quadrado 8x8
  for (uint8_t i = 0; i < 8; ++i) {
      for (uint8_t j = 0; j < 8; ++j) {
          ssd1306_pixel(ssd, x + i, y + j, true);  // Acende o pixel
      }
  }
}

void desenha_molde(ssd1306_t *ssd) {
  // Limpa o display
  ssd1306_fill(ssd, false);

  // 1. Moldura principal (borda afastada 2 pixels)
  ssd1306_rect(ssd, 2, 2, ssd->width-4, ssd->height-4, true, false);

  // 2. Cabeçalho
  ssd1306_draw_string(ssd, "OHMIMETRO", 32, 8);
  ssd1306_hline(ssd, 10, ssd->width-10, 20, true); // Linha sob o título

  // 3. Área de medição
  ssd1306_draw_string(ssd, "RESISTOR:", 10, 30);
  ssd1306_draw_string(ssd, "1.5K", 80, 30); // Exemplo de valor

  // 4. Desenho do resistor (simplificado)
  // Terminais
  ssd1306_hline(ssd, 20, 40, 50, true);  // Terminal esquerdo
  ssd1306_hline(ssd, 85, 105, 50, true); // Terminal direito
  
  // Corpo (com 3 faixas coloridas simuladas)
  ssd1306_rect(ssd, 40, 45, 45, 10, true, false); // Contorno
  ssd1306_vline(ssd, 50, 45, 55, true);  // Faixa 1
  ssd1306_vline(ssd, 60, 45, 55, true);  // Faixa 2
  ssd1306_vline(ssd, 70, 45, 55, true);  // Faixa 3

  // 5. Rodapé
  ssd1306_hline(ssd, 10, ssd->width-10, ssd->height-15, true);
  ssd1306_draw_string(ssd, "MEDIR", 50, ssd->height-10);

  // Atualiza o display
  ssd1306_send_data(ssd);
}

void desenha_molde_completo(ssd1306_t *ssd) {
  // Limpa o display
  ssd1306_fill(ssd, false);

  // 1. Moldura principal (3px de margem)
  ssd1306_rect(ssd, 3, 3, 122, 58, true, false);

  // 2. Cabeçalho
  ssd1306_draw_string_escala(ssd, "Semaforo inteligente", 3, 6, 0.8);
  ssd1306_hline(ssd, 10, 118, 16, true);  // Linha divisória
  ssd1306_hline(ssd, 10, 118, 26, true);
  // Atualiza o display
  ssd1306_send_data(ssd);
}

void ssd1306_divide_em_4_linhas(ssd1306_t *ssd) {
    // Limpa o display
    ssd1306_fill(ssd, false);

    // Moldura ao redor
    ssd1306_rect(ssd, 0, 0, ssd->width, ssd->height, true, false);

    // Calcula posições das linhas divisórias
    uint8_t espacamento = ssd->height / 4;

    // Desenha 3 linhas horizontais para dividir em 4 partes
    for (uint8_t i = 1; i < 4; ++i) {
        ssd1306_hline(ssd, 0, ssd->width - 1, i * espacamento, true);
    }

    // Atualiza o display com as linhas
    ssd1306_send_data(ssd);
}