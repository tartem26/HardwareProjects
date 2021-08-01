void InitializeBluetooth(bool (*OnReceiveInputCallback)(int code[DIGIT_AMOUNT]), void (*OnReceiveNewPasswordCallback)(int code[DIGIT_AMOUNT]));
void HandleBluetooth();
bool ProcessIncomingSerial(String *buf);
