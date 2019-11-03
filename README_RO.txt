(!) Explicațiile din README trebuie coroborate comentariilor ce însoțesc codul,
pentru a putea înțelege în totalitate implementarea.

Tema se bazează pe implementarea laboratorului de multiplexare I/O, la care se
adaugă utilizarea numeroaselor structuri de date tip STL din C++ (hashmap-uri,
set-uri, liste etc.) în scopul stocării mesajelor și clienților abonați la
diferitele topicuri. Întreaga temă aplică tehnici de programare defensivă,
evitându-se pe cât posibil modalităţi prin care aplicaţia să ajungă în stări
nedefinite, prin verificarea input-urilor, parsarea mesajelor primite, afişarea
de mesaje ajutătoare sau, în cazul erorilor majore de conexiune, închiderea
programului cu mesaje sugestive.


Detalii Server:

Există două hashmap-uri: unul pentru clienţi şi ID-urile lor, unul pentru mesaje,
grupate pe topicuri.
Setarea parametrilor de lucru ai server-ului s-a realizat ca în laboratoare.
În plus, s-a setat şi reutilizarea unor IP-uri şi PORT-uri deja utilizate şi s-a
realizat dezactivarea algoritmului Nagle pentru toate conexiunile TCP.
La multiplexare, s-au luat pe rând cazurile:
1) Se citeşte o comandă de la tastatură - singura permisă este 'exit', care este
urmată, automat, de trimiterea acesteia şi către clienţii TCP, pentru închiderea
lor corectă.
2) Conexiune TCP nouă pe socket-ul de listen: când este alocat un socket pentru 
conexiunea server-client TCP, este adăugat în mulţimea read_fds, după care sunt 
analizate posibilităţile:
-client existent - verificăm să nu existe deja conectat un alt client TCP cu acelaşi
ID, caz în care trimitem către acela comanda 'exit' şi păstrăm doar clientul nou
conectat; altfel, clientul este reconectat după o perioadă de inactivitate şi îi
trimitem, din hashmap, mesajele de la topicurile la care a fost abonat cu SF=1
3) Mesaj nou de la client UDP - caz în care analizăm natura topicului:
-topic nou - creăm topicul (nefiind nimeni abonat, primul mesaj se va pierde)
-topic existent - trimitem direct mesajul către clienţii abonaţi conectaţi sau
adăugăm mesajul primit în hashmap-ul de mesaje corespunzător topicului respectiv,
pentru a-l putea trimite la reconectarea clienţilor abonaţi cu SF=1 şi neconectaţi.
Fiecare mesaj are un TTL (time-to-live) care să indice durata de viață - numărul de 
instanțe care mai sunt necesar de trimis. Acest număr scade cu o unitate la fiecare
trimitere a unei instanţe (s-a mai conectat un client care trebuia să primească mesajul,
trimitem mesaj şi decrementăm TTL al mesajului din hashmap). Când acesta ajunge zero, 
mesajele sunt eliminate din memorie. Locaţia (indexul) la care sunt stocate mesajele
de trimis pentru fiecare client sunt stocate într-o listă a fiecărui client în
hashmap-ul clients, unde există intrări de forma <topic, index_mesaj>, astfel ştiind
exact locaţia mesajului de trimis în hashmap-ul topics.
4) Comandă primită de la un client TCP conectat - se identifică ID-ul clientului în
hashmap-ul clients şi se parsează comanda. Clienţii sunt abonaţi sau dezabonaţi de la
diferitele topicuri, cu SF-ul cerut. În cazul în care se cere abonarea la un topic
inexistent, acesta se creează.


Detalii subscriber:

Baza este asemenea laboratorului, cu noile proprietăţi setate ca în cazul server-ului.
Multiplexarea se realizează între tastatură - când se primesc comenzi (exit, subscribe,
unsubscribe), care sunt verificate, există hint-uri pentru utilizator în cazul
incorectitudinii sau incompletitudinii acestora - şi server, de la care se primesc
mesajele trimise de clienţii UDP pentru topicurile la care subscriber-ul este abonat.
La primirea comenzii 'exit' de la server, clientul TCP se închide, în celelalte cazuri
fiind vorba de mesaje ce respectă diferitele formate precizate în cerinţă, realizându-se
extragerea datelor, conversia acestora la host-byte-order, aplicarea unei strategii
defensive în parsarea acestora (ex. s-a asigurat existenţa unui null-terminator pentru
string-uri) şi afişarea la consolă a mesajului primit, în forma prezentată în enunţ.

Modalităţi de rulare:
./server <PORT_DORIT>
./subscriber <ID_Client> <IP_Server> <Port_Server>

Comenzi acceptate de clienți:
-subscribe topic [SF] - SF unu pentru reținerea mesajelor în hash, zero altfel
-unsubcribe topic
-exit
