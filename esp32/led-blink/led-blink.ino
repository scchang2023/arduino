#define GREEN 0
void setup() {
    // put your setup code here, to run once:
    pinMode(GREEN, OUTPUT);
    pinMode(2, OUTPUT);
}

void loop() {
    // put your main code here, to run repeatedly:
    digitalWrite(GREEN, HIGH);
    digitalWrite(2, HIGH);
    delay(500);
    digitalWrite(GREEN, LOW);
    digitalWrite(2, LOW);
    delay(500);
}
