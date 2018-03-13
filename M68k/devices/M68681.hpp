//
// The Motorola 68681 DUART
//

#ifndef M68K_DEVICES_M68681_HPP_
#define M68K_DEVICES_M68681_HPP_

#include <string>
#include <sys/types.h>

#include "Framework/BasicDevice.hpp"

#ifdef BSVC_QT_GUI
  #include "Widgets/duartportdockwidget.h"
#endif

class M68681 : public BasicDevice {
public:
#ifdef BSVC_QT_GUI
  M68681(const std::string &args, BasicCPU &cpu, QWidget* parent = NULL);
#else
  M68681(const std::string &args, BasicCPU &cpu);
#endif
  ~M68681();

  // Returns true iff the address maps into the device
  bool CheckMapped(Address addr) const override;

  // Returns the lowest address used by the device
  Address LowestAddress() const override { return base_address; }

  // Returns the highest address used by the device
  Address HighestAddress() const override {
    return base_address + offset_to_first_register +
           15 * offset_between_registers;
  }

  // Gets a byte from the device
  Byte Peek(Address addr) override;

  // Puts a byte into the device
  void Poke(Address addr, Byte c) override;

  // Resets the DUART.
  void Reset() override;

  // The BasicDevice's InterruptAcknowledge does not handle vectored
  // interrupts so we'll have to change it.
  int InterruptAcknowledge(unsigned int level) override;

  // Handles the DUART's events.
  void EventCallback(int type, void *pointer) override;

private:
  Byte MR1A; // Mode register 1 A
  Byte MR2A; // Mode register 2 A

  Byte SRA;  // Status register A
  Byte CSRA; // Clock-select register A

  // No readable register at this address..
  Byte CRA;  // Command register A

  Byte RBA;  // Receiver buffer A
  Byte TBA;  // Transmitter buffer A

  Byte IPCR; // Input port change register
  Byte ACR;  // Auxiliary control register

  Byte ISR;  // Interrupt status register
  Byte IMR;  // Interrupt mask register

  Byte CUR;  // Counter mode: counter MSB
  Byte CTUR; // Counter/timer uppper register

  Byte CLR;  // Counter mode: counter LSB
  Byte CTLR; // Counter/timer lower register

  Byte MR1B; // Mode register 1 B
  Byte MR2B; // Mode register 2 B
  Byte SRB;  // Status register B
  Byte CSRB; // Clock-select register B
  Byte CRB;  // Command register B
  Byte RBB;  // Receiver buffer B
  Byte TBB;  // Transmitter buffer B

  Byte IVR; // Interrupt-vector register

  Byte mr1a_pointer; // Determines MR1A/MR2A
  Byte mr1b_pointer; // Determines MR1B/MR2B

  Byte receiver_a_state;    // State of receiver A
  Byte transmitter_a_state; // State of transmitter A
  Byte receiver_b_state;    // State of receiver B
  Byte transmitter_b_state; // State of transmitter B

  static long baudrate_table[32]; // Table of times for baud rates

#ifdef _WIN32
  void* coma_read_pipe;
  void* coma_write_pipe;
  void* comb_read_pipe;
  void* comb_write_pipe;
  void* coma_proc;
  void* comb_proc;
#else
  int coma_read_id;  // Pipe to command for port a
  int coma_write_id; // Pipe to command for port a
  int comb_read_id;  // Pipe to command for port b
  int comb_write_id; // Pipe to command for port b
  pid_t coma_pid;    // Proccess ID for port a command
  pid_t comb_pid;    // Proccess ID for port b command
#endif
  
  Address base_address;             // Base address of the DUART
  size_t offset_to_first_register;  // Offset to the first registers
  size_t offset_between_registers;  // Offset to between registers
  unsigned long interrupt_level;    // The interrupt level sent to CPU
  
#ifdef _WIN32
  bool StartPortCommand(const std::string &command, bool std_flag,
                              void* &read_pipe_id, void* &write_pipe_id,
                              void* &pid);
#else
  bool StartPortCommand(const std::string &command, bool std_flag, int &read,
                        int &write, pid_t &pid);
#endif

  void SetInterruptStatusRegister();

#ifdef BSVC_QT_GUI
private:
  DUARTPortDockWidget* dockPortA = NULL;
  DUARTPortDockWidget* dockPortB = NULL;
#endif
};

#endif  // M68K_DEVICES_M68681_HPP_
