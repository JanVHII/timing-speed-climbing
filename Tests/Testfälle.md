# **Simuliere Fälle/Szenarien**

## **1) Normaler Footpad-Start, korrekter Ablauf**

### **Szenario**

- Athlet betritt Fußpad.

- Wartet \> 5s.

- Countdown läuft, erster Ton für 0,2s bei 880 Hz, zweiter Ton 1s nach Beginn des ersten Tons für 0,2s bei 880Hz, dritter Ton nach 2s für 0,1s bei 1760 Hz.

- Löst 0,5s nach Beginn des letzten Startton das Fußpad.

- Berührt später den Anschlag 15s nach Beginn des letzten Starttons.

### **Erwartetes Ergebnis**

- Kein Fehlstart.

- Zwischen letztem Start-Ton und Anschlag wird die laufende Zeit angezeigt.

- Nach Anschlag oben wird zunächst die Zeit des Lauf von 15s in der zweiten Zeile gezeigt.

- In der ersten Zeile wird zusätzlich die Reaktionszeit von 0,5s anzeigt.

- Die Zeiten bleiben auf dem Display bis zum nächsten Start.

- Das System geht in den Zustand Ready zurück.

## **2) Fehlstart vor dem Beginn des letzten Startton**

### **Szenario**

- Athlet steht auf dem Fußpad für \>5s.

- Countdown läuft.

- Athlet löst das Fußpad zu früh aus, 1s nach dem Beginn des ersten Startton.

### Erwartetes Ergebnis

- Fehlstart.

- Anzeige des Fehlstarts auf dem Display und Signalton 1568 Hz für mind. 4S

- Das Display zeigt außerdem an um wie viel der Start zu früh war: 1,1s

- Das System kehrt in den Status Ready zurück

## 3) Fehlstart vor dem Beginn des letzten Startton, mehr Zeiten

### Szenario

- Wie Szenario 2, aber simuliere 3 andere zufällig gewählte Zeiten im Frühstart-Zeitfenster vor dem Beginn des ersten Starttons.  

### Erwartetes Ergebnis

- Wie Szenario 2 nur mit anderen Zeiten.

## 4) Fehlstart während dem letzten Startton

### Szenario

- Athlet steht auf dem Fußpad für \>5s.

- Countdown läuft.

- Athlet löst das Fußpad zu früh aus, 0,05s nach dem Beginn des letzten Startton.

### Erwartetes Ergebnis

- Fehlstart.

- Anzeige des Fehlstarts auf dem Display und Signalton 1568 Hz für mind. 4S

- Das Display zeigt außerdem an um wie viel der Start zu früh war: 0,05s

- Das System kehrt in den Status Ready zurück

## **5) Athlet steigt vor Ablauf der 5 s wieder vom Fußpad**

### **Szenario**

- Fußpad wird betreten.

- Vor Countdown-Beginn wird wieder losgelassen.

### Erwartetes Ergebnis

- Das System geht in den Zustand Ready zurück

## 6) Taster-Start, korrekter Ablauf

### Szenario

- Athlet betätigt den Start-Taster

- Wartezeit von 5s, damit sich der Athlet am Start bereit machen kann.

- Starttöne werden ausgegeben.

- Athlet berührt später den Anschlag 20s nach Beginn des letzten Starttons.

### Erwartetes Ergebnis

- Kein Fehlstart, da im Tasterstart nicht erkennbar.

- Zwischen letztem Start-Ton und Anschlag wird die laufende Zeit angezeigt.

- Nach Anschlag oben wird zunächst die Zeit des Lauf von 20s in der zweiten Zeile gezeigt.

- Es wir keine Reaktionszeit angezeigt.

- Die Zeiten bleiben auf dem Display bis zum nächsten Start.

- Das System geht in den Zustand Ready zurück.

## **7) Taster-Stopp**

### **Szenario**

- Prüfe Szenario 1 und 6 mit Taster-Stopp statt Anschlag oben am Sensor. 

- Für den Taster-Stopp mit nach Taster-Start der Taster erneut gedrückt werden.

### Erwartetes Ergebnis

- Wie in Szenario 1 und 6

## **7) Kein Anschlag, kein Taster-Stopp**

### **Szenario**

- Start erfolgt korrekt.

- Weder Zielkontakt noch Taster-Stopp treten ein.

### Erwartetes Ergebnis

- Zeit läuft beliebig lange weiter. Ein Abbruch erfolgt ggfs. mit Taster-Stopp durch den Athleten.

# **Prüfung auf undefinierte oder problematische Zustände**

- Prüfung ob der Taster korrekt entprellt ist und durch zu langes oder zu kurzes betätigen des Tasters kein Störung des normalen Ablaufs vorkommt.

- Formal undefinierte Zustände

- Problematische fachliche Zustände


