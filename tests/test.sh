
#Teste recebendo aquivo grande
(printf "POST / HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 10\r\n\r\n"; sleep 5; printf "0123456789") | nc localhost 8080