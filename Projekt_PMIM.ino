#include "LedControl.h"
#include "stdint.h"
#define ButtonPin 7
#define EchoPin 8
#define TrigPin 9
#define LiczbaPomiarow (500)
int buttonState = 0;
volatile static uint16_t pomiary[LiczbaPomiarow];     // Tworzymy tablicę pomiarów
LedControl lc = LedControl(12, 11, 10, 1);

void setup() {
  Serial.begin (9600);
  pinMode(TrigPin, OUTPUT);                           // Ultradzwieki jako wyjście
  pinMode(EchoPin, INPUT);                            // Ultradzwieki jako wejscie
  pinMode(ButtonPin, INPUT_PULLUP);                   // Przycisk jako wejście
  lc.shutdown(0, false);                              // Wyświetlacz MAX7219 początkowo jest w fazie oszczędzania energii, należy go uruchomić
  lc.setIntensity(0, 8);                              // Ustawienie jasności jako średnią
  lc.clearDisplay(0);                                 // Czyszczenie wyświetlacza
}

void loop() {                                         // Główna pętla
  buttonState = digitalRead(ButtonPin);               // Sprawdź status przycisku
  if (buttonState == LOW) {                           // Jeżeli przycisk jest naciśniety]
    kreski();
    buttonState = digitalRead(ButtonPin);             // Sprawdz nowy stan przycisku
    if (buttonState == HIGH) {                        // Jezeli nowy stan to puszczenie przycisku
      unsigned long endTime = millis() + 5000;        // POPRAWIĆ NA 5000 Ustaw, że po 5 sekundach pomiar ma się zakończyć
      int i = 0;
      while (millis() <= endTime && i<500) {          // Pętla trwająca do czasu uzupełnienia 500 pomiarów lub do skończenia czasu 5 sekund
        pomiary[i] = zmierzOdleglosc();
        i++;
      }
      uint16_t wynik = algorytm(pomiary, i);          // Używamy funkcji algorytm do wyznaczenia średniej pomiarów, wynik jest średnią pomiarów podaną w [mm]
      wyswietl(wynik);
    }
  }
}

uint16_t zmierzOdleglosc() {                           
  float czas;
  float dystans;
  digitalWrite(TrigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(TrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(TrigPin, LOW);
  czas = pulseIn(EchoPin, HIGH);
  dystans = czas / 58.173357;                          // Odwrotność: 0.3438 - prędkość fali w m/ms (343.8 m/s) w 20 stopniach Celcjusza dzielona przez 2 (ponieważ fala przebyła drogę w dwie strony).
                                                       // Dodatkowo wszystko dzielne przez 10, by zamienić milimetry na centymetry [1/(0.3438/2)/10]
  return (uint16_t)(dystans * 10u);                    // Zwracamy liczbę w formacie całkowitym - oszczędzamy pamięć
}

void kreski() {                                        // Wyswietla migające 4 kreski
  for (int j = 0; j < 4; j++) {
    lc.setChar(0, j, '-', false);
  }
  delay(250);
  lc.clearDisplay(0);
  delay(250);
}

void wyswietl(uint16_t odpowiedz) {                    // Wyświetla otrzymany wynik
  double odp = (double)(odpowiedz) / 10;               // Zamiana na typ zmiennoprzecinkowy
  int przed_przecinkiem;
  int po_przecinku;
  przed_przecinkiem = (int)odp;
  po_przecinku = odp * 10 - przed_przecinkiem * 10;

  int ones, tens, hundreds;
  ones = przed_przecinkiem % 10;
  przed_przecinkiem = przed_przecinkiem / 10;
  tens = przed_przecinkiem % 10;
  przed_przecinkiem = przed_przecinkiem / 10;
  hundreds = przed_przecinkiem;

  lc.setDigit(0, 3, (byte)hundreds, false);             // Wypisz wynik cyfra po cyfrze
  lc.setDigit(0, 2, (byte)tens, false);
  lc.setDigit(0, 0, (byte)po_przecinku, false);
  while (buttonState == HIGH) {                         // Pętla wyświetlająca kropkę i cyfrę jedności
    lc.setDigit(0, 1, (byte)ones, false);
    lc.setChar(0, 1, '.', false);
    buttonState = digitalRead(ButtonPin);               // Przerywana w czasie gdy przycisk zmienia swój stan
  }
}

void sort(uint16_t a[], int s) {                        // Funkcja sortująca tablicę
  for (int j = 0; j < (s - 1); j++) {
    for (int o = 0; o < (s - (j + 1)); o++) {
      if (a[o] > a[o + 1]) {
        double t = a[o];
        a[o] = a[o + 1];
        a[o + 1] = t;
      }
    }
  }
}


uint16_t algorytm(uint16_t pomiary[LiczbaPomiarow], int iloscDobrych) {
  sort(pomiary, LiczbaPomiarow);                                               // Następuje posorttowanie tablicy
  int32_t suma = 0;
  uint16_t srednia = 0;
  uint16_t ileZer = LiczbaPomiarow - iloscDobrych;                             // W przypadku gdy czas pomiarów pozwolił na mniej niż 500 pomiarów, odejmujemy odpowiednią ilość zer, aby wiedzieć ile właściwych wyników znajduje się w tablicy
  for (int j = ileZer + 5; j < LiczbaPomiarow - 5; j++) {                      // Sumujemy wszystkie pomiary poza 5 najmniejszymi i 5 największymi (pozbywamy się tzw błedów grubych - zaburzeń)
    suma += pomiary[j];
  }
  srednia = suma / (iloscDobrych - 10);                                        // Średnią obliczamy przez podzielenie wyników przez liczbę właściwych pomiarów pomniejszoną o 10
  return srednia;
}
