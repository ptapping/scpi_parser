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
  // Trim leading whitespace
  while (strchr(" \r\n\t", *token)) token++;
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

//SCPI_Parser member functions

void SCPI_Parser::setDeviceID(const char* id_string) {
  strncpy(device_id, id_string, SCPI_MAX_BUFFER);
  // Ensure device_id is always null terminated
  device_id[SCPI_MAX_BUFFER - 1] = '\0';
}

char* SCPI_Parser::getDeviceID() {
  return device_id;
}

void SCPI_Parser::addToken(char *token) {
  #ifdef SCPI_DEBUG
  SCPI_DEBUG_DEVICE.println("-> addToken(" + String(token) + ")");
  #endif // SCPI_DEBUG
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
      #ifdef SCPI_DEBUG
      SCPI_DEBUG_DEVICE.println("<- Added token[" + String(tokens_size_) + "] = " + String(stored_token));
      #endif // SCPI_DEBUG
      tokens_size_++;
    } else {
      #ifdef SCPI_DEBUG
      SCPI_DEBUG_DEVICE.println("<- Max # tokens reached, can't add " + String(token));
      #endif // SCPI_DEBUG
    }
  } else {
    #ifdef SCPI_DEBUG
    SCPI_DEBUG_DEVICE.println("<- Existing token " + String(token));
    #endif // SCPI_DEBUG
  }
  // Restore suffix char if token was modified
  if (token_suffix != token) token_suffix[0] = suffix_char;
}

uint32_t SCPI_Parser::getCommandCode(SCPI_Commands& commands) {
  #ifdef SCPI_DEBUG
  SCPI_DEBUG_DEVICE.print("-> getCommandCode(");
  for (uint8_t i = 0; i < commands.size(); i++)
    SCPI_DEBUG_DEVICE.print(String(commands[i]) + (i == commands.size() - 1 ? "" : ", "));
  SCPI_DEBUG_DEVICE.println("), tree_code_ = " + String(tree_code_));
  #endif // SCPI_DEBUG
  uint32_t code = tree_code_;
  char* header;
  for (uint8_t i = 0; i < commands.size(); i++) {
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
        code += j + 1;
        break;
      }
    }
    if (!isToken) {
      #ifdef SCPI_DEBUG
      SCPI_DEBUG_DEVICE.println("<- getCommandCode(...) = 0");
      #endif // SCPI_DEBUG
      return 0;
    }
  }
  //code++;
  if (header[strlen(header) - 1] == '?') code ^= 0x80000000;
  #ifdef SCPI_DEBUG
  SCPI_DEBUG_DEVICE.println("<- getCommandCode() = " + String(code) + " == " + String(code & 0x7fffffff) + ((code & 0x80000000) ? "?" : ""));
  #endif // SCPI_DEBUG
  return code;
}

void SCPI_Parser::setCommandTreeBase(const __FlashStringHelper* tree_base) {
  strcpy_P(msg_buffer, (const char *) tree_base);
  this->setCommandTreeBase(msg_buffer);
}

void SCPI_Parser::setCommandTreeBase(const char* tree_base) {
  #ifdef SCPI_DEBUG
  SCPI_DEBUG_DEVICE.println("-> setCommandTreeBase(" + String(tree_base) + ")");
  #endif // SCPI_DEBUG
  if (strlen(tree_base) > 0) {
    SCPI_Commands tree_tokens(tree_base);
    for (uint8_t i = 0; i < tree_tokens.size(); i++)
      this->addToken(tree_tokens[i]);
    tree_code_ = 0;
    // Don't allow tree code to be a query, or else it inverts query flag on all subsequent commands
    tree_code_ = this->getCommandCode(tree_tokens) & 0x7fffffff;
  } else {
    tree_code_ = 0;
  }
  #ifdef SCPI_DEBUG
  SCPI_DEBUG_DEVICE.println("<- setCommandTreeBase(" + String(tree_base) + "), tree_code_ = " + String(tree_code_));
  #endif // SCPI_DEBUG
}

void SCPI_Parser::registerCommand(const __FlashStringHelper* command, SCPI_caller_t caller) {
  strcpy_P(msg_buffer, (const char *) command);
  this->registerCommand(msg_buffer, caller);
}

void SCPI_Parser::registerCommand(const char* command, SCPI_caller_t caller) {
  #ifdef SCPI_DEBUG
  SCPI_DEBUG_DEVICE.println("-> registerCommand(" + String(command) + ")");
  #endif // SCPI_DEBUG
  SCPI_Commands command_tokens(command);
  for (uint8_t i = 0; i < command_tokens.size(); i++)
    this->addToken(command_tokens[i]);
  uint32_t code = this->getCommandCode(command_tokens);
  for (uint8_t i = 0; i < codes_size_; i++) {
    if (code == valid_codes_[i]) {
      // Command code is a duplicate, don't add it
      #ifdef SCPI_DEBUG
      SCPI_DEBUG_DEVICE.println("<- Not registering duplicate command: " + String(command) + " = " + String(code));
      #endif // SCPI_DEBUG
      return;
    }
  }
  valid_codes_[codes_size_] = code;
  callers_[codes_size_] = caller;
  codes_size_++;
  #ifdef SCPI_DEBUG
  SCPI_DEBUG_DEVICE.println("<- Registered command: " + String(command) + " = " + String(code));
  #endif // SCPI_DEBUG
}

void SCPI_Parser::execute(char* message, Stream &interface) {
  #ifdef SCPI_DEBUG
  SCPI_DEBUG_DEVICE.println("-> execute(" + String(message) + ")");
  #endif // SCPI_DEBUG
  // Intercept SCPI required commands, such as "*IDN?" and handle internally
  // Note, being case sensitive here due to lazyness
  if (strcmp(message, "*IDN?") == 0) {
    interface.println(device_id);
  } else if (strcmp(message, "HELP?") == 0) {
    printCommands(interface);
  } else {
    // Check for matching user commands
    tree_code_ = 0;
    SCPI_Commands commands(message);
    SCPI_Parameters parameters(commands.not_processed_message);
    uint32_t code = this->getCommandCode(commands);
    for (uint8_t i = 0; i < codes_size_; i++)
      if (valid_codes_[i] == code)
        (*callers_[i])(commands, parameters, interface);
  }
}

char* SCPI_Parser::getMessage(Stream& interface, char* term_chars) {
  while (interface.available()) {
    msg_buffer[msg_counter++] = interface.read();
    msg_buffer[msg_counter] = '\0';
    if (strchr(term_chars, msg_buffer[msg_counter - 1]) != NULL) {
      // Terminator character received
      msg_buffer[msg_counter - 1] = '\0';
      if (msg_counter > 1) {
        // A useful sized message
        msg_counter = 0;
        return msg_buffer;
      } else {
        // Ignore messages with only terminator chars, eg when \r\n
        msg_counter = 0;
        return NULL;
      }
    }
    if (msg_counter >= SCPI_MAX_BUFFER - 1) {
      // Buffer full, return what we have anyway
      msg_counter = 0;
      return msg_buffer;
    }
  }
  // No message, or message incomplete
  return NULL;
}

void SCPI_Parser::processInput(Stream& interface, char* term_chars) {
  char* message = this->getMessage(interface, term_chars);
  if (message != NULL) {
    this->execute(message, interface);
  }
}

void SCPI_Parser::printDebugInfo() {
  SCPI_DEBUG_DEVICE.println(F("*** DEBUG INFO ***"));
  SCPI_DEBUG_DEVICE.print(F("TOKENS: "));
  SCPI_DEBUG_DEVICE.println(tokens_size_);
  for (uint8_t i = 0; i < tokens_size_; i++) {
    SCPI_DEBUG_DEVICE.print(F("  "));
    SCPI_DEBUG_DEVICE.println(String(tokens_[i]));
    SCPI_DEBUG_DEVICE.flush();
  }
  SCPI_DEBUG_DEVICE.println();
  SCPI_DEBUG_DEVICE.print(F("VALID CODES: "));
  SCPI_DEBUG_DEVICE.println(codes_size_);
  for (uint8_t i = 0; i < codes_size_; i++) {
    SCPI_DEBUG_DEVICE.print(F("  "));
    SCPI_DEBUG_DEVICE.println(String(valid_codes_[i]) + " = " + String(valid_codes_[i] & 0x7fffffff) + String(valid_codes_[i] & 0x80000000 ? " ?" : ""));
    SCPI_DEBUG_DEVICE.flush();
  }
  SCPI_DEBUG_DEVICE.println(F("******************"));
}

void SCPI_Parser::printCommands(Stream& interface) {
  interface.print(F("Commands are:\n"));
  for (uint8_t i = 0; i < codes_size_; i++) {
    interface.print("  ");
    uint32_t parent_id = ((valid_codes_[i] & 0x7fffffff))/SCPI_MAX_TOKENS;
    if (parent_id > 0) {
      // Somewhat confusing method to "reverse traverse" the token tree.
      int8_t tree_depth = log(parent_id)/log(SCPI_MAX_TOKENS);
      // Integer power calculation, as floating point error in pow() breaks things
      uint32_t divisor = 1;
      for (uint8_t t = 0 ; t < tree_depth; t++) divisor *= SCPI_MAX_TOKENS;
      // Now loop through from the bottom of the tree and work upwards
      for ( ; tree_depth >= 0; tree_depth--) {
        interface.print(String(tokens_[uint8_t(parent_id/divisor) - 1]) + ":");
        parent_id %= divisor;
        divisor /= SCPI_MAX_TOKENS;
      }
    }
    // Append the final node
    uint8_t token_id = (uint8_t)(((valid_codes_[i] & 0x7fffffff) - 1)%SCPI_MAX_TOKENS);
    bool is_query = bool(valid_codes_[i] & 0x80000000);
    interface.print(String(tokens_[token_id]) + (is_query ? "?" : ""));
    interface.print(i == codes_size_ - 1 ? "\r\n" : "\n");
  }
  interface.flush();
}
