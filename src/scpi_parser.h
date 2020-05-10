/*
 * MIT License
 *
 * Copyright (c) 2018 Diego González Chávez
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#ifndef SCPI_parser_h
#define SCPI_parser_h

// Maximum size of command tree and number of parameters
#ifndef SCPI_ARRAY_SIZE
#define SCPI_ARRAY_SIZE 6
#endif 

#ifndef SCPI_MAX_TOKENS
#define SCPI_MAX_TOKENS 15
#endif 

// Maximun number of registered commands 
#ifndef SCPI_MAX_COMMANDS
#define SCPI_MAX_COMMANDS 20
#endif 

#include <Arduino.h>

class SCPI_String_Array {
 public:
  SCPI_String_Array();
  ~SCPI_String_Array();
  char* operator[](const byte index);
  void append(char* value);
  char* pop();
  char* first();
  char* last();
  uint8_t size();
 protected:
  uint8_t size_ = 0;
  char* values_[SCPI_ARRAY_SIZE];
};

class SCPI_Commands : public SCPI_String_Array {
 public:
  SCPI_Commands();
  SCPI_Commands(char* message);
  char* not_processed_message;
};

class SCPI_Parameters : public SCPI_String_Array {
 public:
  SCPI_Parameters();
  SCPI_Parameters(char *message);
  char* not_processed_message;
};

typedef SCPI_Commands SCPI_C;
typedef SCPI_Parameters SCPI_P;
typedef void (*SCPI_caller_t)(SCPI_C, SCPI_P, Stream&);

class SCPI_Parser {
 public:
  void setCommandTreeBase(const char* tree_base);
  void setCommandTreeBase(const __FlashStringHelper* tree_base);
  void registerCommand(const char* command, SCPI_caller_t caller);
  void registerCommand(const __FlashStringHelper* command, SCPI_caller_t caller);
  void execute(char* message, Stream& interface);
  void processInput(Stream &interface, char* term_chars);
  char* getMessage(Stream& interface, char* term_chars);
  void printDebugInfo();
  void printCommands(Stream& interface);
 protected:
  void addToken(char* token);
  uint32_t getCommandCode(SCPI_Commands& commands);
  uint8_t tokens_size_ = 0;
  char *tokens_[SCPI_MAX_TOKENS];
  uint8_t codes_size_ = 0;
  uint32_t valid_codes_[SCPI_MAX_COMMANDS];
  SCPI_caller_t callers_[SCPI_MAX_COMMANDS];
  uint32_t tree_code_ = 1;
  char msg_buffer[64]; //TODO BUFFER_LENGTH
};

#endif
