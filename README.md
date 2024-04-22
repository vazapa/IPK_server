# IPK 2024 - Projekt 2
# IOTA varianta
 

## Úvod

Tento projekt implementuje chatovací server, který umožnuje komunikaci mezi protokoly TCP a UDP.

## Spuštění

Překlad
```console
$ make
```
Spuštení serveru a jeho nastavení parametrů
```console
$ ./ipk24chat-server -l [IP address] -p [port] -d [confirmation timeout] -r [retransmissions]
```
Všechny parametry jsou volitelné, výchozí hodnoty uvedeny v závorkách 
- IP address -> Adresa serveru na které poběží (Výchozí hodnota je 0.0.0.0)
- port -> Port serveru na kterém poběží (Výchozí hodnota je 4567)
- confirmation timeout -> Doba čekání na potrvzení poslané zprávy pomocí UDP (Výchozí hodnota je 250)
- retransmissions -> Maximální počet poslání zprávy pomocí UDP dokud neojde potvrzení o doručení. (Výchozí hodnota je 3)

Vypsaní pomocné zprávy
```console
$ ./ipk24chat-server -h
```
## Soubory
- `main.c`  -> Slouží pouze ke zpracování parametrů z terminálu a spuštění počáteční funkce `server()`. Při spuštení bez pamaretrů jsou přirazeny výchozí hodnoty.
- `server.c`-> Hlavní část programu. Zde se vytváří potřebné sockety, inicializují potřebné struktury a následně se pomocí smyčky `while()` kontroluje aktivitu na daných socketech. Příchozí zprávy se dále zpracovávají podle toho o jaký protokol se jedná.
- `tcp.c`   -> Tato část je zodpovědná za zpracování TCP zpráv.
- `udp.c`   -> Tato část je zodpovědná za zpracování UDP zpráv.

## Chování
- Server dokáže přijímat několik druhů zpráv pro autentikaci uživatele, připojení do jiného kanálu.. 
- Reakcí na tyto zprávy je volání přislušné funkce pro její zpracování, uložení potřebných údajů a případné odeslání odpovědi. 
- Časti příchozích zpráv jsou přiřazeny do proměnných a následně zpracovány
- První vstup od klienta musí být pokus o autentikaci, pro zjednodušení je __vždy__ odeslána kladná reakce. 
- Nyní může klient komunikovat s ostatnímy uživateli a využívat příkazy, zároveň v tento moment jsou do pole struktury `Client` přiřazené potřebné informace 
- Zprávy jsou rozeslány mezi všechny uživatele nehlědě jaký protokol využívají.

### TCP
- Protokol využívající textových zpráv
- Pomocí podmínky se porovnává několík počátečních bytů v poli (`buffer`) s odpovídajícím začátkém možné zprávy.
- Po vytvoření odpovídajícího „welcome" socketu se kontroluje jeho aktivita, pokud je zaznamenána volá se funkce `tcp_accept()`, která zajistí spojení mezi serverem a klientem, také se pro nového uživatele přiřadí nový socket, kde se opět kontroluje aktivita a prochází se pole klientů.

    
### UDP
- Protokol využívající binárních zpráv
- Po vytvoření odpovídajícího socketu se kontroluje jeho aktivita, pokud je zaznamenána, volá se funkce `handle_udp_packet()`. 
- Pomocí podmínky se porovnává počáteční byte pole (`buffer[0]`) s číslem označující typ zprávy. Tato zpráva se potvrdí pomocí funkce `confirm()`. 

### Výstup serveru

### Testování

