#include "remregs_master.h"

// oscillator frequency, needed for __delay_ms()
#define _XTAL_FREQ  32000000

/// UART circular buffer size, must be a power of two for efficiency reasons
#define BUFFER_SIZE 32

// UART circular buffer for reception
static volatile uint8_t idx_in = 0;
static volatile uint8_t idx_out = 0;
static volatile uint8_t UART_buffer[BUFFER_SIZE];

// synchronization state
static __bit sync_state = 0;

/// 8-bit register read
static const uint8_t ROP_READ_8 = 0;
/// 16-bit register read
static const uint8_t ROP_READ_16 = 1;
/// 32-bit register read
static const uint8_t ROP_READ_32 = 2;
/// multibyte register read
static const uint8_t ROP_READ_MB = 3;
/// 8-bit register write
static const uint8_t ROP_WRITE_8 = 4;
/// 16-bit register write
static const uint8_t ROP_WRITE_16 = 5;
/// 32-bit register write
static const uint8_t ROP_WRITE_32 = 6;
/// multibyte register write
static const uint8_t ROP_WRITE_MB = 7;

/// acknowledge
static const uint8_t ACK = 6;
/// negative acknowledge
static const uint8_t NAK = 15;

/// maximum size of a multibyte register
static const uint8_t MAX_MB_SIZE = 29;

/// Is there anything waiting in the UART receive buffer?
#define UART_kbhit() (idx_in != idx_out)

void remregs_UART_ISR()
{
  while (RC2IF) {
    if (!FERR2) {
      UART_buffer[idx_out++] = RCREG2;
      idx_out = idx_out % BUFFER_SIZE;
    } else {
      RCREG2;  // ignore byte with framing error
    }
  }
}

static void UART_clear_oerr()
{
  if (OERR2) {
    di();
    CREN2 = 0;
    CREN2 = 1;
    RC2IE = 1;
    ei();
  }
}

static uint8_t UART_getch()
{
  uint8_t data, timer;
  timer = 0;
  while (idx_in == idx_out) {
    timer++;
    __delay_ms(10);
    if (timer == 200) {
      return 0xff;
    }
    UART_clear_oerr();
    CLRWDT();
  }
  data = UART_buffer[idx_in++];
  idx_in = idx_in % BUFFER_SIZE;
  return data;
}

static __bit UART_read(uint8_t* buf, const uint8_t size)
{
  for (uint8_t i = 0; i < size; i++) {
    buf[i] = UART_getch();
  }
  return 1;
}

static void UART_putch(uint8_t c)
{
  while (!TRMT2);
  TXREG2 = c;
}

__bit remregs_sync()
{
  sync_state = 0;
  uint8_t timer = 0;
  for (uint8_t i = 0; i < 24; i++) {
    UART_putch(0xff);
  }
  UART_putch(0xaa);
  uint8_t b = 0;
  do {
    __delay_ms(1);
    if (UART_kbhit()) {
      b = UART_getch();
    } else {
      timer++;
      CLRWDT();
      if (timer == 250) {
        return 0;
      }
    }
  } while (b != 0xAA && b != 0x55);      // accepts both 0xAA (BioRob radio interface) and 0x55 (as implemented by ARM-side of radio protocol or Arduino version)
  sync_state = 1;
  return 1;
}

static __bit reg_op(const uint8_t op, const uint16_t addr, const uint8_t* data, const uint8_t len)
{
  
  // check if a sync operation is needed
  if (!sync_state) {
    if (!remregs_sync()) {
      return 0;
    }
  }

  // sends the request (2-byte opcode/address plus data if any)
  UART_putch((op << 2) | ((addr & 0x300) >> 8));
  UART_putch(addr & 0xff);
  if (op == ROP_WRITE_MB) {
    UART_putch(len);
  }
  for (uint8_t i = 0; i < len; i++) {
    UART_putch(data[i]);
  }

  // reads the ACK
  uint8_t r = UART_getch();
  if (r == 0xff) {
    sync_state = 0;
    return 0;
  }
  
  if (r == ACK) {
    return 1;
  } else {
    return 0;
  }
}

uint8_t get_reg_8(const uint16_t addr)
{
  if (!reg_op(ROP_READ_8, addr, NULL, 0)) {
    return 0xff;
  }
  return UART_getch();
}

uint16_t get_reg_16(const uint16_t addr)
{
  if (!reg_op(ROP_READ_16, addr, NULL, 0)) {
    return 0xff;
  }
  uint16_t res;
  if (UART_read((uint8_t*) &res, 2)) {
    return res;
  } else {
    return 0xffff;
  }
}

uint32_t get_reg_32(const uint16_t addr)
{
  if (!reg_op(ROP_READ_32, addr, NULL, 0)) {
    return 0xff;
  }
  uint32_t res;
  if (UART_read((uint8_t*) &res, 4)) {
    return res;
  } else {
    return 0xffffffff;
  }
}

__bit get_reg_mb(const uint16_t addr, uint8_t* data, uint8_t* len)
{
  if (!reg_op(ROP_READ_MB, addr, NULL, 0)) {
    return 0;
  }
  *len = UART_getch();
  return UART_read(data, *len);
}

__bit set_reg_8(const uint16_t addr, const uint8_t val)
{
  return reg_op(ROP_WRITE_8, addr, &val, 1);
}

__bit set_reg_16(const uint16_t addr, const uint16_t val){
  return reg_op(ROP_WRITE_16, addr, (uint8_t*) &val, 2);
}

__bit set_reg_32(const uint16_t addr, const uint32_t val)
{
  return reg_op(ROP_WRITE_32, addr, (uint8_t*) &val, 4);
}

__bit set_reg_mb(const uint16_t addr, const uint8_t* data, const uint8_t len)
{
  return reg_op(ROP_WRITE_MB, addr, data, len);
}
