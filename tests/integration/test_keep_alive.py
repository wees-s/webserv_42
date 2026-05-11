import socket

GREEN = "\033[92m"
RED = "\033[91m"
RESET = "\033[0m"

def test_keep_alive():
    target_host = "127.0.0.1"
    target_port = 8080
    
    print(f"[*] Iniciando teste de Keep-Alive em {target_host}:{target_port}...")
    
    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.settimeout(2)
        client.connect((target_host, target_port))
        
        # --- PRIMEIRA REQUISIÇÃO ---
        print("[1/2] Enviando primeira requisição...")
        client.send(b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
        
        # Recebe a primeira resposta
        response1 = client.recv(4096).decode()
        if "200 OK" in response1:
            print(f" {GREEN}✓{RESET} Primeira resposta recebida.")
        else:
            print(f" {RED}✗{RESET} Erro na primeira resposta.")
            return

        # --- SEGUNDA REQUISIÇÃO (No mesmo socket!) ---
        print("[2/2] Enviando segunda requisição no MESMO socket...")
        client.send(b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
        
        # Recebe a segunda resposta
        response2 = client.recv(4096).decode()
        if "200 OK" in response2:
            print(f"\n{GREEN}[PASS]{RESET} Servidor processou múltiplas requisições no mesmo socket!")
        else:
            print(f"\n{RED}[FAIL]{RESET} Servidor fechou a conexão ou não respondeu a segunda requisição.")
            
    except Exception as e:
        print(f"\n{RED}[ERROR]{RESET} Erro: {e}")
    finally:
        client.close()

if __name__ == "__main__":
    test_keep_alive()
