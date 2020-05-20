#include "scpi_parser.h";

SCPI_Parser instr;

// Input and output pin mappings
uint8_t analog_input_pins[] = {A0, A1, A2, A3, A4, A5};
uint8_t analog_output_pins[] = {3, 5, 6, 9, 10, 11};
uint8_t digital_input_pins[] = {2, 4, 7};
uint8_t digital_output_pins[] = {8, 12};

void setup() {
  Serial.begin(115200);

  instr.setDeviceID("SCPI Parser,Instrument Uno,0,0.1");
  instr.registerCommand("Help?", &help);
  instr.setCommandTreeBase("Analog");
    instr.registerCommand(":Output", &analog_value);
    instr.registerCommand(":Input?", &analog_value);
  instr.setCommandTreeBase("Digital");
    instr.registerCommand(":Output", &digital_out_value);
    instr.registerCommand(":Output?", &digital_out_value);
    instr.registerCommand(":Input?", &digital_in_value);
  instr.setCommandTreeBase("Led");
    instr.registerCommand(":State", &set_led_state);
    instr.registerCommand(":State?", &get_led_state);

  // Configure digital input/output pins
  for (uint8_t i = 0; i < sizeof(digital_input_pins); i++)
    pinMode(digital_input_pins[i], INPUT);
  for (uint8_t i = 0; i < sizeof(digital_output_pins); i++)
    pinMode(digital_output_pins[i], OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  instr.processInput(Serial, "\r\n");
}

void help(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  instr.printCommands(interface, false);
  interface.print("Numeric suffixes available:\n");
  interface.print("  Analog:Input1 to " + String(sizeof(analog_input_pins)) + "\n");
  interface.print("  Analog:Output1 to " + String(sizeof(analog_output_pins)) + "\n");
  interface.print("  Digital:Input1 to " + String(sizeof(digital_input_pins)) + "\n");
  interface.println("  Digital:Output1 to " + String(sizeof(digital_output_pins)));
}

void analog_value(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  // Collect numeric suffixes from commands, default if not present is "1"
  long suffixes[commands.size()];
  for (uint8_t i = 0; i < commands.size(); i++) {
    char *suffix = strpbrk(commands[i], "1234567890");
    suffixes[i] = (suffix != NULL) ? String(suffix).toInt() : 1;
  }

  // Use suffix of "Input/Output" to select analog input/output pin, note labelling is one-based
  if (String(commands.last()).endsWith("?")) {
    // Request is a query
    if (suffixes[1] > 0 & suffixes[1] <= sizeof(analog_input_pins)) { // Note sizeof(uint8_t) is 1
      // Valid suffix
      interface.println(analogRead(analog_input_pins[suffixes[1] - 1]));
    } else {
      // Invalid suffix, return empty line
      interface.println();
    }
  } else {
    // Request is a command
    if (suffixes[1] > 0 & suffixes[1] <= sizeof(analog_output_pins) & parameters.size() > 0) {
      // Valid pin suffix, and parameter(s) present
      analogWrite(analog_output_pins[suffixes[1] - 1], constrain(String(parameters[0]).toInt(), 0, 255));
    }
  }
}

void digital_out_value(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  // Collect numeric suffixes from commands, default if not present is "1"
  long suffixes[commands.size()];
  for (uint8_t i = 0; i < commands.size(); i++) {
    char *suffix = strpbrk(commands[i], "1234567890");
    suffixes[i] = (suffix != NULL) ? String(suffix).toInt() : 1;
  }

  // Use suffix of "Output" to select digital output pin, note labelling is one-based
  if (String(commands.last()).endsWith("?")) {
    // Request is a query, we can also read from a digital output pin
    if (suffixes[1] > 0 & suffixes[1] <= sizeof(digital_output_pins)) {
      // Valid suffix
      interface.println(digitalRead(digital_output_pins[suffixes[1] - 1]));
    } else {
      // Invalid suffix, return empty line
      interface.println();
    }
  } else {
    // Request is a command
    if (suffixes[1] > 0 & suffixes[1] <= sizeof(digital_output_pins) & parameters.size() > 0) {
      // Valid pin suffix, and parameter(s) present
      digitalWrite(digital_output_pins[suffixes[1] - 1], constrain(String(parameters[0]).toInt(), 0, 1));
    } else {
      // Invalid suffix or no parameter, return empty line
      interface.println();
    }
  }
}

void digital_in_value(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  // Collect numeric suffixes from commands, default if not present is "1"
  long suffixes[commands.size()];
  for (uint8_t i = 0; i < commands.size(); i++) {
    char *suffix = strpbrk(commands[i], "1234567890");
    suffixes[i] = (suffix != NULL) ? String(suffix).toInt() : 1;
  }

  // Use suffix of "Input" to select digital input pin, note labelling is one-based
  if (suffixes[1] > 0 & suffixes[1] <= sizeof(digital_input_pins)) {
    // Valid suffix
    interface.println(digitalRead(digital_input_pins[suffixes[1] - 1]));
  } else {
    // Invalid suffix, just return an empty line
    interface.println();
  }
}

void set_led_state(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (parameters.size() > 0) {
    uint8_t led_state = constrain(String(parameters[0]).toInt(), 0, 1);
    digitalWrite(LED_BUILTIN, led_state);
  }
}

void get_led_state(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  interface.println(digitalRead(LED_BUILTIN));
}
