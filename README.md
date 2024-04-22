# IPK 2024 - Projekt 2
# IOTA varianta
 

## Úvod

Tento projekt implementuje server, který umožnuje komunikaci mezi protokoly TCP a UDP.

## Spuštění

Překlad
```console
$ make
```
Spuštení serveru a nastavení jeho parametrů
```console
$ ./ipk24chat-server -l [IP address] -p [port] -d [confirmation timeout] -r [retransmissions]
```
Všechny parametry jsou volitelné, výchozí hodnoty uvedeny v závorkách 
- IP address -> Adresa serveru na které poběží (Výchozí hodnota je 0.0.0.0)
- port -> Port serveru na kterém poběží (Výchozí hodnota je 4567)
- confirmation timeout -> Doba čekání na potrvzení poslané zprávy pomocí UDP (Výchozí hodnota je 250)
- retransmissions -> Maximální počet poslání zprávy pomocí UDP dokud nedojde potvrzení o doručení. (Výchozí hodnota je 3)

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
- Reakcí na tyto zprávy je volání příslušné funkce pro její zpracování, uložení potřebných údajů a případné odeslání odpovědi. 
- Časti příchozích zpráv jsou přiřazeny do proměnných a následně zpracovány
- První vstup od klienta musí být pokus o autentikaci, pro zjednodušení je __vždy__ odeslána kladná reakce. 
- Nyní může klient komunikovat s ostatními uživateli a využívat příkazy, zároveň v tento moment jsou do pole struktury `Client` přiřazené potřebné informace 
- Zprávy jsou rozesílány mezi všechny uživatele, a to bez ohledu na to, který protokol používají.

### TCP
- Protokol využívající textových zpráv
- Pomocí podmínky se porovnává několík počátečních bytů v poli (`buffer`) s odpovídajícím začátkém možné zprávy.
- Po vytvoření odpovídajícího „welcome" socketu se kontroluje jeho aktivita, pokud je zaznamenána volá se funkce `tcp_accept()`, která zajistí spojení mezi serverem a klientem, také se pro nového uživatele přiřadí nový socket, kde se opět kontroluje aktivita a prochází se pole klientů.

    
### UDP
- Protokol využívající binárních zpráv
- Po vytvoření odpovídajícího socketu se kontroluje jeho aktivita. Pokud je zaznamenána, volá se funkce `handle_udp_packet()`
- Pomocí podmínky se porovnává počáteční byte pole (`buffer[0]`) s číslem označující typ zprávy. Tato zpráva se potvrdí pomocí funkce `confirm()`

### Testování

Server byl testován pomocí několika terminalů s klienty a jedním terminálem pro server. Dále jsem využíval `Wireshark` pro sledování komunikace a zjištování chyb

![Wireshark](https://git.fit.vutbr.cz/xzaple40/ipk24chat-server/raw/branch/main/term.png)

![Wireshark](https://git.fit.vutbr.cz/xzaple40/ipk24chat-server/raw/branch/main/ws1.png)

![Wireshark](https://git.fit.vutbr.cz/xzaple40/ipk24chat-server/raw/branch/main/ws2.png)

### Bibliografie

[RFC2119] Bradner, S. _Key words for use in RFCs to Indicate Requirement Levels_ [online]. March 1997. [cited 2024-02-11]. DOI: 10.17487/RFC2119. Available at: https://datatracker.ietf.org/doc/html/rfc2119

[RFC5234] Crocker, D. and Overell, P. _Augmented BNF for Syntax Specifications: ABNF_ [online]. January 2008. [cited 2024-02-11]. DOI: 10.17487/RFC5234. Available at: https://datatracker.ietf.org/doc/html/rfc5234

[RFC9293] Eddy, W. _Transmission Control Protocol (TCP)_ [online]. August 2022. [cited 2024-02-11]. DOI: 10.17487/RFC9293. Available at: https://datatracker.ietf.org/doc/html/rfc9293

[RFC894] Hornig, C. _A Standard for the Transmission of IP Datagrams over Ethernet Networks_ [online]. April 1984. [cited 2024-02-14]. DOI: 10.17487/RFC894. Available at: https://datatracker.ietf.org/doc/html/rfc894

[RFC791] Information Sciences Institute, University of Southern California. _Internet Protocol_ [online]. September 1981. [cited 2024-02-14]. DOI: 10.17487/RFC791. Available at: https://datatracker.ietf.org/doc/html/rfc791

[RFC768] Postel, J. _User Datagram Protocol_ [online]. March 1997. [cited 2024-02-11]. DOI: 10.17487/RFC0768. Available at: https://datatracker.ietf.org/doc/html/rfc768

[RFC1350] Sollins, D. _The TFTP Protocol (Revision 2)_ [online]. July 1992. [cited 2024-02-12]. DOI: 10.17487/RFC1350. Available at: https://datatracker.ietf.org/doc/html/rfc1350

