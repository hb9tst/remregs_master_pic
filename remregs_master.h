#ifndef __REMREGS_MASTER_H
#define	__REMREGS_MASTER_H

#include <xc.h>
#include <stdint.h>

/**
 * \file   remregs_master.h
 * \brief  Header for 8-bit PIC (XC8 compiler) implementation of the master side of remregs
 * \author Alessandro Crespi
 * \date   August 2018
 */

/**
 * ISR for UART reception, must be called by application main interrupt
 * handler on UART receive interrupt to ensure buffering of received data.
 */
void remregs_UART_ISR(void);


/// Synchronizes the communication between the PIC and the remregs slave
__bit remregs_sync();

/** \brief Reads an 8-bit register
  * \param addr The address of the register (0 - 1023)
  * \return The read value (0x00 - 0xff) or 0xff on failure
  */
uint8_t get_reg_8(const uint16_t addr);

/** \brief Reads a 16-bit register
  * \param addr The address of the register (0 - 1023)
  * \return The read value (0x0000 - 0xffff) or 0xffff on failure
  */
uint16_t get_reg_16(const uint16_t addr);

/** \brief Reads a 32-bit register
  * \param addr The address of the register (0 - 1023)
  * \return The read value (0x00000000 - 0xffffffff) or 0xffffffff on failure
  */
uint32_t get_reg_32(const uint16_t addr);

/** \brief Reads a multibyte register
  * \param addr The address of the register (0 - 1023)
  * \param data A pointer to the output buffer (at least 29 bytes long)
  * \param len Pointer to a variable that will contain the length of the returned data
  * \return 1 if the operation succeeded, 0 if not
  */
__bit get_reg_mb(const uint16_t addr, uint8_t* data, uint8_t* len);


/** \brief Writes an 8-bit register
  * \param addr The address of the register (0 - 1023)
  * \param val The value to write to the register
  * \return 1 if the operation succeeded, 0 if not
  */  
__bit set_reg_8(const uint16_t addr, const uint8_t val);

/** \brief Writes a 16-bit register
  * \param addr The address of the register (0 - 1023)
  * \param val The value to write to the register
  * \return 1 if the operation succeeded, 0 if not
  */  
__bit set_reg_16(const uint16_t addr, const uint16_t val);

/** \brief Writes a 32-bit register
  * \param addr The address of the register (0 - 1023)
  * \param val The value to write to the register
  * \return 1 if the operation succeeded, 0 if not
  */
__bit set_reg_32(const uint16_t addr, const uint32_t val);

/** \brief Writes a multibyte register
  * \param addr The address of the register (0 - 1023)
  * \param data Pointer to the data to write to the register
  * \param len Length of the data to write (0 - 29 bytes)
  * \return 1 if the operation succeeded, 0 if not
  */
__bit set_reg_mb(const uint16_t addr, const uint8_t* data, const uint8_t len);

#endif
