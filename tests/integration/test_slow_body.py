import socket
import time
GREEN = "\033[92m"
RED = "\033[91m"
RESET = "\033[0m"

def test_slow_body():
    target_host = "127.0.0.1"
    target_port = 8080
    body_content = "1234567890" # 10 bytes
    
    print(f"[*] Iniciando teste de Slow Body (POST) em {target_host}:{target_port}...")
    
    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.settimeout(5)
        client.connect((target_host, target_port))
        
        # 1. Envia o Header primeiro
        header = (
            "POST /api/curriculum HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Content-Type: text/plain\r\n"
            f"Content-Length: {len(body_content)}\r\n"
            "\r\n"
        )
        print("[1/2] Enviando cabeçalho do POST...")
        client.send(header.encode())
        
        # 2. Envia o corpo caractere por caractere
        print("[2/2] Enviando corpo lentamente (1 byte por vez)...")
        for char in body_content:
            client.send(char.encode())
            print(f" Enviado: {char}")
            time.sleep(0.2) # 200ms de delay entre cada byte
            
        # 3. Recebe a resposta
        response = client.recv(4096).decode()
        
        if "302 Found" in response or "200 OK" in response:
            print(f"\n{GREEN}[PASS]{RESET} Servidor aguardou o corpo completo corretamente!")
        else:
            print(f"\n{RED}[FAIL]{RESET} Servidor respondeu antes da hora ou deu erro.")
            print("Resposta:", response)
            
    except Exception as e:
        print(f"\n{RED}[ERROR]{RESET} Erro na comunicação: {e}")
    finally:
        client.close()

if __name__ == "__main__":
    test_slow_body()

