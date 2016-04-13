# Arduino-FibocomG510
Przykładowy projekt pokazujący jak używać rozszerzenie GSM od MikroTar.pl z Arduino. 

Funkcje:
- sendATcommand(...): wysyła komendę AT do czipu G510
- sendATcommandS(...): wysyla komendę AT zadaną ilosć razy co podaną ilość milisekund
- chipToggle(): przełącza czip G510 w stan włączony/wyłączony
- chipOn(...): włacza czip,
- chipPresent(): sprawdza, czy czip jest dostępny,
- displayModuleCaps(): wyświetla właściwości czipu.

##Użycie 
W pierwszym kroku należy stworzyć i zainicjować instancję SoftwareSerial, np:
SoftwareSerial ss(12, 8);
...dla nadawania po pinie D8 i odbierania na pinie D12 (zworka A-N ustawina na N).
W funkcji setup, podajcie:

void setup(){
  //...
  ss.begin(9600);
  //...
}

Teraz port szeregowy "ss" jest gotowy do komunikacji z G510.

###Wywołanie sendATcommand()
Funkcja sendATcommand() przyjmuje kilka parametrów:
- SoftwareSerial *ss: wskaźnik do obiektu portu szeregowego, np. &ss,
- char* cmd: bufor, który zawiera komendę AT, np. "AT+CMGF", 
- char* pars: bufor, ktory zawiera parametry do komendy AT, np. "?",
- char*postEcho: bufor, który zostanie wysłany do czipa po znaku zapytania '>', 
- char*bufRet: bufor na łancuch zwracany przez wywołanie funkcji AT,
- int bufRetSize: rozmiar bufora "bufRet", maksymalnie 256 znaków.

###Wywołanie sendATcommandS()
Wywołanie jak dla sendATcommand() z dodatkowymi parametrami:
- byte repeat: ilość powtórzeń komendy,
- int delays: ilość milisekund pomiędzy kolejnymi wywołaniami komendy AT

###Debugowanie
Projekt zawiera 2 dodatkowe makra do debuggowania: 
- debugTron(...) - wyświetla napis na konsoli i wartość
- debugStep() - wyświetla napis na konsoli

Zwróćcie uwagę na deklarację:

*#define DEBUG ...*

Teraz: 

*#define DEBUG true*

...włącza napisy debugowe,

*#define DEBUG false*

...wyłącza napisy debugowe.
