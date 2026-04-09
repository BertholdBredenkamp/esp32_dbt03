# HinweisESP32-C3 SuperMini DBT03 Modul

Ich habe die Programmquellen für die Arduino V2 IDE angepasst. Bei meinem Terminal musste die Abfrage für die Einstellungen auf die Taste 5 gelegt werden. Das erste Zeichen war immer die 1 die dann sofort beim Start die Einstellungen startete.

Bei den ersten Einstellungen dauert es ca. 20 Sekunden bis die Taste erkannt wird. Davor ist im Terminal ein kurzer Ton zu hören.

Die IP-Adresse des BTX Servers kann sowohl als Zahlenfolge wie auch als DNS Name eingegeben werden.

Die Verbindung zum BTX Terminal ist ein 7 poliges DIN Kabel. Bitte am Terminal schauen ob dort eine Buchse oder ein Stecker verbaut ist.

## Einstellungen

Die Einstellungen können nicht das Zeichen "#" enthalten, sie werden mit der Taste 5 innerhalb von 10 Sekunden nach dem Start der BTX Verbindung am  Terminal aufgerufen


## Hinweise

Das Layout basiert auf einem ESP32-C3 SuperMini Board. Bei meinem Modell musste ich die Sendeleistung verringern. Es gibt auch Empfehlungen eine Antenne zusätzlich aufzulöten. Bei anderen Ausführungen des ESP Moduls soll der Fehler nicht vorhanden sein.
