**_Progresso:_**

**_Apr 22_** - Criado sistema inicial de pastas.
___
**_Apr 22_** - Html basico para testes (index, contacts, posts).
___
**_Apr 22_** - Estrutura basica Request.hpp
___
**_Apr 22_** - 
**Componente:** Sockets / Bootstrap do projeto  

**Resumo Técnico:**  
- Criado `include/Request.hpp` com estrutura inicial de `Request` (method/path/version/body + `headers` em `std::map`).  
- Adicionado `sandbox.cpp` com servidor TCP mínimo (socket→bind→listen→accept) respondendo um payload HTTP/1.1 fixo via `write()`.  
- Atualizado `Makefile` com pipeline básico de build (C++98, `objs/`, output colorido) e `.gitignore` para ignorar `.cursorrules`.  
- Binário/artefato local `teste_socket` apareceu como untracked (não versionado).  
  
**Decisões de Arquitetura:**  
- Fluxo de socket validado com syscalls clássicas: `socket()`, `bind()`, `listen()`, `accept()`, `write()`, `close()`.  
- Prova de vida HTTP feita com resposta estática para validar camada de rede antes do parser/roteamento.  
- [NÃO IMPLEMENTADO] I/O não-bloqueante e multiplexação (`select`/`poll`/`epoll`) ainda não entrou no protótipo.  
  
**Desafios:**  
- Nenhum bug documentado hoje; próxima dor esperada é migrar de `accept()`/I/O bloqueante para loop de eventos e buffers parciais (reads/writes incompletos).
___
**_Apr 24_** - Merge user1 && user2.
___
**_Apr 27_** - Integração Socket + Parser
**Componente:** Integração de componentes existentes

**Resumo Técnico:**
- Integrado o parser de request (`Request.cpp/Request.hpp`) com o socket (`sandbox.cpp`)
- Modificado `sandbox.cpp` para ler request do socket via `read()` e passar pelo parser `Request`
- Atualizado `Makefile` para incluir `Request.cpp` na compilação
- Consolidado todos os testes em `main.cpp` (teste parser com string fixa + teste integração socket+parser)
- Validado com `curl http://localhost:8080/sobre` - parser funcionou corretamente

**Testes Realizados:**
- Teste 1: Parser com string fixa "GET /sobre HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\n\r\n"
- Teste 2: Integração socket real + parser (request recebida via curl, parseada e impressa)

**Decisões de Arquitetura:**
- Centralização de testes em `main.cpp` para facilitar validação
- Buffer de 4096 bytes para leitura do socket (suficiente para requests básicas)
- Uso de C++98 conforme padrão do projeto

**Desafios:**
- Nenhum bug encontrado na integração
- Parser funciona corretamente com requests reais do curl
___
**_Apr 27 (tarde)_** - Refatoração e Loop de Conexões
**Componente:** Arquitetura e servidor persistente

**Resumo Técnico:**
- Refatorado para separar responsabilidades: `SocketServer.cpp/SocketServer.hpp` (apenas socket) e `ParserRequest.cpp/ParserRequest.hpp` (apenas parser)
- Renomeado classe `Request` para `ParserRequest` para maior clareza
- Renomeado arquivo `Request.cpp` para `parserRequest.cpp`
- Implementado loop infinito em `socket_server()` para aceitar múltiplas conexões consecutivas
- Adicionado signal handler (`SIGINT`) para encerramento graceful com Ctrl+C
- `server_fd` mantido aberto fora do loop, apenas `client_fd` fechado a cada iteração
- Parser movido para dentro do loop do socket (arquitetura simplificada)
- `main.cpp` simplificado para apenas chamar `socket_server()`

**Testes Realizados:**
- Teste 1: Múltiplas requests consecutivas (`/pagina1`, `/pagina2`) - servidor permanece ativo
- Teste 2: Parser funcionando corretamente para cada request
- Teste 3: Encerramento com Ctrl+C funciona gracefulmente

**Decisões de Arquitetura:**
- Parser dentro do loop do socket por simplicidade (pode ser refatorado depois para callback)
- Uso de `volatile sig_atomic_t` para flag de shutdown (thread-safe com signal handler)
- Resposta HTTP fixa mantida por enquanto (servir HTMLs pendente)

**Desafios:**
- Nenhum bug encontrado no loop de conexões
- Servidor aceita múltiplas requests sem problemas
___
**_Apr 27 (noite)_** - Refatoração para Classes e Tratamento de Métodos
**Componente:** Arquitetura OOP e tratamento de HTTP methods

**Resumo Técnico:**
- Transformado `SocketServer` de função para classe com membros `server_fd` e `client_fd`
- Criada classe `TrateRequest` para separar lógica de tratamento de métodos HTTP
- Implementado tratamento de métodos: GET (servir HTMLs), POST, DELETE
- `TrateRequest` recebe `client_fd` via construtor para enviar respostas
- GET `/` serve `www/index.html`, outros paths servem arquivos correspondentes
- Respostas de erro: 404 (arquivo não encontrado), 405 (método não suportado)
- Headers HTTP corretos: `Content-Type: text/html`, `Content-Length`

**Testes Realizados:**
- Teste 1: `GET /` → serve `index.html` corretamente (837 bytes)
- Teste 2: `GET /contacts.html` → serve `contacts.html` corretamente (1690 bytes)
- Teste 3: `POST /teste` → retorna "POST received"
- Teste 4: Múltiplas requests consecutivas funcionam

**Decisões de Arquitetura:**
- Separação clara: `SocketServer` (rede), `ParserRequest` (parsing), `TrateRequest` (lógica de negócio)
- Classe `SocketServer` com métodos `setup()`, `handleConnection()`, `run()`
- `TrateRequest` decide ação baseado no método no construtor
- Uso de `new/delete` para buffer de arquivo (C++98, sem smart pointers)

**Desafios:**
- Correção de includes em `SocketServer.hpp` (removidos includes de implementação)
- Arquivos renomeados para PascalCase (`SocketServer.cpp`, `ParserRequest.cpp`, `TrateRequest.cpp`)
___
