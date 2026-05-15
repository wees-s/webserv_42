import socket
import time
# Cores para o terminal
GREEN = "\033[92m"
RED = "\033[91m"
RESET = "\033[0m"

def test_fragmentation():
    target_host = "127.0.0.1"
    target_port = 8080
    
    print(f"[*] Iniciando teste de fragmentação em {target_host}: {target_port}...")
    
    try:
        # 1. Conecta ao servidor
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.settimeout(5) # Não deixa o teste travar se o servidor não responder
        client.connect((target_host, target_port))
         
        # 2. Envia a primeira parte (Incompleta)
        # Cortamos no meio da palavra "HTTP"
        print("[1/2] Enviando fragmento: 'GET / HTT'...")
        client.send(b"GET / HTT")
        
        # 3. Pausa para forçar o servidor a voltar para o poll()
        time.sleep(0.5)
        
        # 4. Envia o resto da requisição
        print("[2/2] Enviando o restante do cabeçalho...")
        client.send(b"P/1.1\r\nHost: localhost\r\n\r\n")
        
        # 5. Recebe e valida a resposta
        response = client.recv(4096).decode()
        
        if "200 OK" in response:
            print(f"\n{GREEN}[PASS]{RESET} Servidor reconstruiu a requisição corretamente!")
            print("-" * 30)
            print(response.split('\n')[0]) # Mostra a primeira linha da resposta
            print("-" * 30)
        else:
            print(f"\n{RED}[FAIL]{RESET} Resposta inesperada do servidor.")
            print("Resposta recebida:", response)
    except Exception as e:
        print(f"\n{RED}[ERROR]{RESET} Falha ao conectar ou comunicar: {e}")
    finally:
        client.close()

if __name__ == "__main__":
    test_fragmentation()
