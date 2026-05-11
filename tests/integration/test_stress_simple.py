import socket
import threading

GREEN = "\033[92m"
RED = "\033[91m"
RESET = "\033[0m"

def make_request(id, results):
    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.settimeout(2)
        client.connect(("127.0.0.1", 8080))
        client.send(b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
        response = client.recv(1024).decode()
        if "200 OK" in response:
            results.append(True)
        else:
            results.append(False)
        client.close()
    except:
        results.append(False)

def test_concurrency():
    num_threads = 50
    threads = []
    results = []
    
    print(f"[*] Abrindo {num_threads} conexões simultâneas...")
    
    for i in range(num_threads):
        t = threading.Thread(target=make_request, args=(i, results))
        threads.append(t)
        t.start()
        
    for t in threads:
        t.join()
        
    successes = results.count(True)
    failures = results.count(False)
    
    print(f"\nResultados:")
    print(f"{GREEN} Sucessos: {successes}{RESET}")
    print(f"{RED} Falhas: {failures}{RESET}")
    
    if successes == num_threads:
        print(f"\n{GREEN}[PASS]{RESET} Servidor lidou com a carga sem erros!")
    else:
        print(f"\n{RED}[FAIL]{RESET} Algumas conexões falharam.")

if __name__ == "__main__":
    test_concurrency()