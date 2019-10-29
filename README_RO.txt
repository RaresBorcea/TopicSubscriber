=============== Tema 3 PC ===============
Borcea Rareș Ioan; Grupa 324CC; 2018-2019

(!) Explicațiile din README trebuie coroborate comentariilor ce însoțesc codul,
pentru a putea înțelege în totalitate implementarea.

Tema se bazează pe implementarea laboratorului dedicat protocolului HTTP, având
funcţiile de bază uşor modificate, astfel încât acestea să suporte adăugarea în
request-uri a cookie-urilor, a unui token JWT, să ţină cont de nişte valori default
în cazul în care unii parametrii lipsesc (programare defensivă) etc. La acestea se
adaugă utilizarea numeroaselor funcţii-helper, dedicate parsării unui răspuns de la
server în sensul obţinerii anumitor parametrii, precum cookie-uri, parametrii URL etc.
Toate aceste funcţii sunt prezente şi documentate în fişierele requests.c (pentru
generarea request-urilor GET şi POST), respectiv helpers.c - deschiderea conexiunii,
primirea răspunsurilor etc.


Detalii client.c:

Fiecare etapă a fost abordată pe rând (şi unica modalitate, de altfel :) ). Există
un pattern comun fiecăreia, în sensul: deschidem conexiunea cu un server, creăm un
request, trimitem mesajul, primim răspuns, închidem conexiunea.
Răspunsul este analizat, fiind extrase câmpurile/obiectele necesare etapei următoare
din obiectul JSON primit (sau cookie-urile şi token-ul din header-e).
Valorile din câmpul 'data' au fost, ori de câte ori a fost posibil, extrase automat,
prin funcţia 'get_params', care a extras toţi parametrii URL şi i-a concatenat
într-un string de adăugat la adresa host-ului. S-au extras numele, respectiv valoarea
fiecărui field.
Cookie-urile au fost extrase cu ajutorul funcţiei 'extract_cookies'.
Tipurile de applicaţii trimise către server au fost: formulare sau JSON, selecţia
realizându-se în cadrul funcţiilor de generare a request-ului, pe baza unui parametru.
În cadrul etapei 5, s-a trimis întâi request-ul către serverul OpenWeatherMap.
Adresa IP a fost obţinută prin 'getaddrinfo', ca în laboratorul dedidcat DNS.
Din răspunsul primit s-a făcut selecţia JSON-ului, care a fost forward-at către serverul
temei, răspunsul final (din fericire, '200 OK') fiind afişat la consolă, însoţit de
textul din body.


Modalităţi de rulare:

Makefile-ul păstrează regulile din laboratorul 10, existând:
-make run;
-make clean.