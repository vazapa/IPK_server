## Omezení

### UDP
- /join u udp funguje pouze pokud není více uživatelů se stejným `display_name`
- Zpráva o odchodu klienta z kanálu není implementována 
- Dynamický port není implementován
- Timeout a retries nejsou implementovány
### TCP
- Detekování `\r\n` pro TCP stream není implementováno
### Ostatní
- Nejsou implementovány kontroly zpráv od uživatele, ani unikátnost údaje `username` 