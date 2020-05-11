/*
 * MIT License
 *
 * Copyright (c) 2018 Diego González Chávez, Patrick Tapping
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


#include "scpi_parser.h";

// SCPI_String_Array member functions

SCPI_String_Array::SCPI_String_Array() {}

SCPI_String_Array::~SCPI_String_Array() {}

char* SCPI_String_Array::operator[](const uint8_t index) {
  return values_[index];
}

void SCPI_String_Array::append(char* value) {
  if (size_ < SCPI_ARRAY_SIZE) {
    values_[size_] = value;
    size_++;
  }
}

char* SCPI_String_Array::pop() {
  if (size_ > 0) {
    size_--;
    return values_[size_];
  } else {
    return NULL;
  }
}

char* SCPI_String_Array::first() {
  if (size_ > 0) {
    return values_[0];
  } else {
    return NULL;
  }
}

char* SCPI_String_Array::last() {
  if (size_ > 0) {
    return values_[size_ - 1];
  } else {
    return NULL;
  }
}

uint8_t SCPI_String_Array::size() {
  return size_;
}

// SCPI_Commands member functions

SCPI_Commands::SCPI_Commands() {}

SCPI_Commands::SCPI_Commands(char *message) {
  char* token = message;
  // Trim leading spaces
  while (isspace(*token)) token++;
  // Discard parameters and multicommands
  not_processed_message = strpbrk(token, " \t;");
  if (not_processed_message != NULL) {
    not_processed_message += 1;
    token = strtok(token, " \t;");
    token = strtok(token, ":");
  } else {
    token = strtok(token, ":");
  }
  // Strip using ':'
  while (token != NULL) {
    this->append(token);
    token = strtok(NULL, ":");
  }
}

// SCPI_Parameters member functions

SCPI_Parameters::SCPI_Parameters() {}

SCPI_Parameters::SCPI_Parameters(char* message) {
  char* parameter = message;
  // Discard parameters and multicommands
  not_processed_message = strpbrk(parameter, ";");
  if (not_processed_message != NULL) {
    not_processed_message += 1;
    parameter = strtok(parameter, ";");
    parameter = strtok(parameter, ",");
  } else {
    parameter = strtok(parameter, ",");
  }
  // Strip using ':'
  while (parameter != NULL) {
    while(isspace(*parameter)) parameter++;
    this->append(parameter);
    parameter = strtok(NULL, ",");
  }
}


//SCPI_Registered_Commands member functions

void SCPI_Parser::addToken(char *token) {
  // Strip '?' or numeric suffix from end if needed
  char *token_suffix = strpbrk(token, "1234567890?");
  char suffix_char = '\0';
  if (token_suffix != NULL) {
    // Suffix present. Insert terminator bvte, but remember character replaced
    suffix_char = token_suffix[0];
    token_suffix[0] = '\0';
  }
  //add the token
  // TODO Match long or short versions of tokens
  bool already_added = false;
  for (uint8_t i = 0; i < tokens_size_; i++)
    already_added ^= (strcmp(token, tokens_[i]) == 0);
  if (!already_added) {
    if (tokens_size_ < SCPI_MAX_TOKENS) {
      char *stored_token = new char [strlen(token) + 1];
      strcpy(stored_token, token);
      tokens_[tokens_size_] = stored_token;
      tokens_size_++;
    }
  }
  // Restore suffix char if token was modified
  if (token_suffix != token) token_suffix[0] = suffix_char;
}

uint32_t SCPI_Parser::getCommandCode(SCPI_Commands& commands) {
  uint32_t code = tree_code_ - 1;
  char* header;
  for (int i = 0; i < commands.size(); i++) {
    code *= SCPI_MAX_TOKENS;
    header = commands[i];
    bool isToken;
    for (uint8_t j = 0; j < tokens_size_; j++) {
      size_t ss = 0; // Token short size
      while (isupper(tokens_[j][ss])) ss++;
      size_t ls = strlen(tokens_[j]); // Token long size
      size_t hs = strlen(header); // Header size
      // But ignore any suffix in header string length
      char *header_suffix = strpbrk(header, "1234567890?");
      if (header_suffix != NULL) hs -= strlen(header_suffix);

      isToken = true;
      if (hs == ss && ss > 0) {
        // short token length matches, compare strings
        for (uint8_t k  = 0; k < ss; k++)
          isToken &= (toupper(header[k]) == tokens_[j][k]);
      } else if (hs == ls) {
        // long token length matches, compare strings
        for (uint8_t k  = 0; k < ls; k++)
          isToken &= (toupper(header[k]) == toupper(tokens_[j][k]));
      } else {
        isToken = false;
      }
      if (isToken) {
        code += j;
        break;
      }
    }
    if (!isToken) return 0;
  }
  if (header[strlen(header) - 1] == '?') code ^= 0x80000000;
  return code+1;
}

void SCPI_Parser::setCommandTreeBase(const __FlashStringHelper* tree_base) {
  strcpy_P(msg_buffer, (const char *) tree_base);
  this->setCommandTreeBase(msg_buffer);
}

void SCPI_Parser::setCommandTreeBase(const char* tree_base) {
  if (strlen(tree_base) > 0) {
    SCPI_Commands tree_tokens(tree_base);
    for (uint8_t i = 0; i < tree_tokens.size(); i++)
      this->addToken(tree_tokens[i]);
    tree_code_ = 1;
    // Don't allow tree code to be a query, or else it inverts query flag on all subsequent commands
    tree_code_ = this->getCommandCode(tree_tokens) & 0x7fffffff;
  } else {
    tree_code_ = 1;
  }
}

void SCPI_Parser::registerCommand(const __FlashStringHelper* command, SCPI_caller_t caller) {
  strcpy_P(msg_buffer, (const char *) command);
  this->registerCommand(msg_buffer, caller);
}

void SCPI_Parser::registerCommand(const char* command, SCPI_caller_t caller) {
  SCPI_Commands command_tokens(command);
  for (uint8_t i = 0; i < command_tokens.size(); i++)
    this->addToken(command_tokens[i]);
  uint32_t code = this->getCommandCode(command_tokens);
  for (uint8_t i = 0; i < codes_size_; i++) {
    if (code == valid_codes_[i]) {
      // Command code is a duplicate, don't add it
      return;
    }
  }
  valid_codes_[codes_size_] = code;
  callers_[codes_size_] = caller;
  codes_size_++;
}

void SCPI_Parser::execute(char* message, Stream &interface) {
  tree_code_ = 1;
  SCPI_Commands commands(message);
  SCPI_Parameters parameters(commands.not_processed_message);
  uint32_t code = this->getCommandCode(commands);
  for (uint8_t i = 0; i < codes_size_; i++)
    if (valid_codes_[i] == code)
      (*callers_[i])(commands, parameters, interface);
}

char* SCPI_Parser::getMessage(Stream& interface, char* term_chars) {
  uint8_t msg_counter = 0;
  msg_buffer[msg_counter] = '\0';

  bool continous_data = false;
  unsigned long last_data_millis = millis();
  do {
    if (interface.available()) {
        continous_data = true;
        last_data_millis = millis();
        msg_buffer[msg_counter] =  interface.read();

        //TODO check msg_counter overflow
        ++msg_counter;
        msg_buffer[msg_counter] = '\0';

        if (strstr(msg_buffer, term_chars) != NULL) {
          msg_buffer[msg_counter - strlen(term_chars)] =  '\0';
          break;
        }
    } else { //No chars aviable yet
      if ((millis() - last_data_millis) > 10) // 10 ms without new data
        continous_data = false;
    }
  } while (continous_data);
  if (continous_data)
    return msg_buffer;
  else
    return NULL;
}

void SCPI_Parser::processInput(Stream& interface, char* term_chars) {
  char* message = this->getMessage(interface, term_chars);
  if (message != NULL) {
    this->execute(message, interface);
  }
}

void SCPI_Parser::printDebugInfo(Stream& interface) {
  interface.println(F("*** DEBUG INFO ***"));
  interface.println();
  interface.print(F("TOKENS :"));
  interface.println(tokens_size_);
  for (uint8_t i = 0; i < tokens_size_; i++) {
    interface.print(F("  "));
    interface.println(String(tokens_[i]));
    interface.flush();
  }
  interface.println();
  interface.println(F("VALID CODES :"));
  for (uint8_t i = 0; i < codes_size_; i++) {
    interface.print(F("  "));
    interface.println(String(valid_codes_[i]) + " = " + String(valid_codes_[i] & 0x7fffffff) + String(valid_codes_[i] & 0x80000000 ? " ?" : ""));
    interface.flush();
  }
  interface.println();
  interface.println(F("*******************"));
  interface.println();
}

void SCPI_Parser::printCommands(Stream& interface) {
  interface.println(F("Commands are:"));
  for (uint8_t i = 0; i < codes_size_; i++) {
    interface.print("  ");
    uint32_t parent_id = ((valid_codes_[i] & 0x7fffffff) - 1)/SCPI_MAX_TOKENS;
    if (parent_id > 0) {
      // Somewhat confusing method to "reverse traverse" the token tree.
      int8_t tree_depth = log(parent_id)/log(SCPI_MAX_TOKENS);
      // Integer power calculation, as floating point error in pow() breaks things
      uint32_t divisor = 1;
      for (uint8_t t = 0 ; t < tree_depth; t++) divisor *= SCPI_MAX_TOKENS;
      // Now loop through from the bottom of the tree and work upwards
      for ( ; tree_depth >= 0; tree_depth--) {
        interface.print(String(tokens_[uint8_t(parent_id/divisor)]) + ":");
        parent_id %= divisor;
        divisor /= SCPI_MAX_TOKENS;
      }
    }
    // Append the final node
    uint8_t token_id = (uint8_t)(((valid_codes_[i] & 0x7fffffff) - 1)%SCPI_MAX_TOKENS);
    bool is_query = bool(valid_codes_[i] & 0x80000000);
    interface.println(String(tokens_[token_id]) + (is_query ? "?" : ""));
  }
  interface.flush();
}
