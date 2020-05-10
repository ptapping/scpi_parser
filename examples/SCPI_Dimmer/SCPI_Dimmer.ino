#include "scpi_parser.h";

SCPI_Parser my_instrument;
int brightness = 0;
const int ledPin = 9;
const int intensity[11] = {0, 3, 5, 9, 15, 24, 38, 62, 99, 159, 255};

void setup()
{
  my_instrument.registerCommand("*IDN?", &identify);
  my_instrument.setCommandTreeBase("SYSTem:LED");
    my_instrument.registerCommand(":BRIGhtness", &setBrightness);
    my_instrument.registerCommand(":BRIGhtness?", &setBrightness);
    my_instrument.registerCommand(":BRIGhtness:INCrease", &incDecBrightness);
    my_instrument.registerCommand(":BRIGhtness:DECrease", &incDecBrightness);
  
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  analogWrite(ledPin, 0);
}

void loop()
{
  my_instrument.processInput(Serial, "\r");
}

void identify(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  interface.println("SCPI Parser,Arduino SCPI Dimmer,#00,v0.1");
}

void setBrightness(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  // For simplicity no bad parameter check is done.
  if (parameters.size() > 0) {
    brightness = constrain(String(parameters[0]).toInt(), 0, 10);
    analogWrite(ledPin, intensity[brightness]);
  }
}

void getBrightness(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  interface.println(String(brightness, DEC));
}

void incDecBrightness(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  String last_header = String(commands.last());
  last_header.toUpperCase();
  if (last_header.startsWith("INC")) {
    brightness = constrain(brightness + 1, 0, 10);
  } else { // "DEC"
    brightness = constrain(brightness - 1, 0, 10);
  }
  analogWrite(ledPin, intensity[brightness]);
}
