#ifndef log_h
#define log_h


void printId(unsigned long id) {
  if (id == VALUE_UNSET) {
    Serial.print("UNSET");
  } else {
    Serial.print(id);
  }
}

void print(const Payload &payload) {
  Serial.print("Payload {");
  Serial.print(" messageId=");
  Serial.print(payload.messageId);
  Serial.print(" id=");
  printId(payload.id);
  Serial.print(" toId=");
  printId(payload.toId);
  Serial.print(" gateCode=");
  Serial.print(payload.gateCode);
  Serial.print(" requestACK=");
  Serial.print(payload.requestACK);
  Serial.print(" command=");
  switch (payload.command) {
    case RUNNING:
      Serial.print("RUNNING");
      break;
    case NO_LONGER_RUNNING:
      Serial.print("NO_LONGER_RUNNING");
      break;
    case HELLO_WORLD:
      Serial.print("HELLO_WORLD");
      break;
    case WELCOME:
      Serial.print("WELCOME");
      break;
    case ACK:
        Serial.print("ACK");
        break;
    case UNKNOWN:
        Serial.print("UNKNOWN");
        break;
    default:
      Serial.print("UNDEFINED");
  }
  // Serial.print(" debug=");
  // Serial.print(payload.debugMessage);
  Serial.print(" ");
  Serial.print(" }");
}

void println(const Payload &payload) {
  print(payload);
  Serial.println(" ");
}



#endif