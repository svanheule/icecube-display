#ifndef SERIAL_H
#define SERIAL_H

// USART interrupt handling
#define COMMAND_FRAME 'A'
#define COMMAND_DEMO 'D'
#define COMMAND_TEST_RING 'R'
#define COMMAND_TEST_SNAKE 'S'
#define COMMAND_GET_ID 'I'

void init_serial_port();

// TODO Rename usart_state to something more appropriate
enum usart_state_t {
    USART_WAIT
  , USART_FRAME
  , USART_DEMO
  , USART_TEST_RING
  , USART_TEST_SNAKE
  // TODO Add diagnostics
};

enum usart_state_t get_usart_state();

#endif
