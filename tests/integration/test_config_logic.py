import socket

GREEN = "\033[92m"
RED = "\033[91m"
RESET = "\033[0m"

def send_request(method, path):
    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.settimeout(2)
        client.connect(("127.0.0.1", 8080))
        request = f"{method} {path} HTTP/1.1\r\nHost: localhost\r\n\r\n"
        client.send(request.encode())
        response = client.recv(4096).decode()
        client.close()
        return response
    except Exception as e:
        return str(e)

def test_config():
    print(f"[*] Iniciando testes de lógica de configuração...")
    
    # 1. Teste 404 Customizado
    print("\n[1/2] Testando 404 Not Found (URL inexistente)...")
    res404 = send_request("GET", "/url_que_nao_existe_mesmo")
    if "404 Not Found" in res404:
        # Verifica se o corpo da resposta contém algo que identifique sua página customizada
        # (Ajuste 'Custom 404' para algo que realmente esteja no seu arquivo 404.html)
        print(f" {GREEN}✓{RESET} Recebeu 404 corretamente.")
    else:
        print(f" {RED}✗{RESET} Falha: Esperava 404, mas veio algo diferente.")
    
    # 2. Teste 405 Method Not Allowed
    print("\n[2/2] Testando 405 Method Not Allowed (DELETE em local proibido)...")
    # Na sua config, /upload só permite POST
    res405 = send_request("DELETE", "/upload")
    if "405 Method Not Allowed" in res405:
        print(f" {GREEN}✓{RESET} Recebeu 405 corretamente.")
    else:
        print(f" {RED}✗{RESET} Falha: Esperava 405 para DELETE em /upload.")
        print("Resposta recebida:", res405.split('\n')[0])

if __name__ == "__main__":
    test_config()