**_Progresso:_**

**Data:** 2026-05-04  
**Componente:** SocketServer — `poll()`, I/O não bloqueante, escrita em `POLLOUT`

**Resumo Técnico:**  
- **Pipeline (ordem de execução em runtime):** loop com `poll()` sobre `std::vector<struct pollfd>`; `fcntl(F_SETFL, O_NONBLOCK)` no socket de listen e em cada cliente aceito; `recv()` acumula em `_client_buffers` até `\r\n\r\n`; resposta enfileirada em `_client_responses[fd]` e `events` passa de `POLLIN` para `POLLOUT`; `handleClientWrite()` usa `send()` parcial (remove o prefixo enviado da string) até a fila esvaziar; `closeConnection()` limpa `_client_buffers` e `_client_responses` e remove o FD do vetor; ao encerrar o processo, o destrutor fecha os FDs ainda listados em `_poll_fds`.  
- **Integração no ponto “request completo”:** `ParserRequest` / `TrateRequest` comentados; resposta HTTP fixa só para validar o pipeline acima.  
- **No mesmo período, eixo repositório (linha do tempo de commits, não ordem de syscalls):** binário alvo `webserv`; README com compilação e testes; merges de parser/config; ajustes em `TrateRequest` e `www` junto ao módulo de socket.

**Decisões de Arquitetura:**  
- `poll(2)` com timeout de 1000 ms para permitir saída cooperativa via `g_running` após SIGINT.  
- Multiplexação única (single-thread) sem threads, alinhado ao subject.  
- Varredura do vetor de `pollfd` de trás para frente ao processar eventos, compatível com `erase` ao fechar conexões.  
- Separação explícita entre fase de leitura (`POLLIN`) e fase de escrita (`POLLOUT`) para lidar com `send()` parcial sem bloquear o loop.

**Desafios:**  
- Reativar `ParserRequest` + `TrateRequest` preenchendo `_client_responses` em vez da resposta estática; validar `Content-Length` e corpo (stub atual pode divergir do tamanho real do payload).  
- POST / bodies grandes: após `\r\n\r\n` ainda pode ser necessário acumular bytes até satisfazer `Content-Length` (fluxo anterior na entrada **May 3** abaixo).  
- `accept()` em socket não bloqueante: em erro, considerar `errno == EAGAIN` vs erro fatal (comportamento depende de carga e kernel).

___
**_May 3_** - Upload de arquivos e persistência de dados
**Componente:** Upload de arquivos, persistência de dados e limpeza de código

**Resumo Técnico:**
- **SocketServer:** Modificado `handleConnection()` para ler body completo em loop conforme `Content-Length`, resolvendo `ERR_CONNECTION_RESET` em uploads grandes
- **Front-end:** Adicionado atributo `data-photo` aos elementos de foto (classico.html, profissional.html), removido código não utilizado (photo-link, form-photoUrl, experiences/education/skills)
- **ifGet:** Adicionado suporte a `application/json` em `getContentType()` para servir curriculum.json
- **ifPost:** Implementado parsing de `multipart/form-data` para upload de arquivos, salvando em `www/uploads/` e atualizando `curriculum.json` com URL da foto
- **ifDelete:** Criado método para deletar `curriculum.json` e limpar arquivos de `www/uploads/`
- **Makefile:** Ajustado `fclean` para remover `www/data/curriculum.json` e `www/uploads/*`
- **Pastas:** Criado `www/data/` para armazenar curriculum.json e default_curriculum.json, `www/uploads/` para arquivos enviados

**Testes Realizados:**
- Teste 1: Upload de arquivo (452KB) - funciona sem `ERR_CONNECTION_RESET` ✓
- Teste 2: Foto salva como `photoUrl` no JSON ✓
- Teste 3: Limpar dados carrega valores de `default_curriculum.json` ✓
- Teste 4: `fclean` remove arquivos de uploads e curriculum.json ✓

**Decisões de Arquitetura:**
- Leitura em loop com buffer de 4096 bytes até completar body conforme `Content-Length`
- Uso de `system("rm -f www/uploads/*")` para limpar arquivos (simplicidade)
- `photoUrl` em vez de `photo` no JSON para compatibilidade com JavaScript
- `clearAllData()` chama `loadCurriculum()` para carregar valores padrão

**Desafios:**
- `ERR_CONNECTION_RESET` resolvido lendo body completo antes de criar ParserRequest
- Compilação com C++98 (sem `std::stoi`, usando `atoi`)
- Input type="file" não pode ser preenchido programaticamente (limitação de segurança)
___
**_May 1_** - Novo front-end
___
**_Apr 30_** - Implementação POST e Melhorias de Estabilidade
**Componente:** Tratamento de requisições POST e estabilidade do servidor

**Resumo Técnico:**
- Implementado método `ifPost()` em `TrateRequest` para processar formulário de depoimento
- Parser extrai campos `nome` e `depoimento` do body da requisição POST
- Validação: campos vazios → `www/error/depoimento_empty.html` (400 Bad Request)
- Validação: tamanho excedido (nome > 30, depoimento > 200) → `www/error/depoimento_size.html` (400 Bad Request)
- Sucesso → `www/success.html` (200 OK)
- Criados arquivos HTML: `depoimento.html` (formulário), `depoimento_empty.html`, `depoimento_size.html`, `success.html`
- Adicionado `setsockopt(SO_REUSEADDR)` em `SocketServer::setup()` para permitir reutilização imediata da porta 8080
- Implementado `select()` com timeout de 1 segundo antes de `accept()` para evitar bloqueio indefinido
- Loop principal verifica `g_running` a cada segundo, permitindo encerramento graceful

**Testes Realizados:**
- Teste 1: POST válido (`nome=joao&depoimento=ola`) → success.html ✓
- Teste 2: POST com campo vazio (`nome=&depoimento=ola`) → depoimento_empty.html ✓
- Teste 3: POST com nome > 30 caracteres → depoimento_size.html ✓
- Teste 4: Servidor permanece ativo após requisições (não encerra sozinho)
- Teste 5: Reutilização de porta 8080 após encerramento funciona

**Decisões de Arquitetura:**
- Páginas de erro separadas (não query string com JavaScript) por simplicidade
- Validação no servidor (não apenas HTML) por segurança
- `select()` com timeout como solução temporária antes de implementar epoll
- `SO_REUSEADDR` para desenvolvimento (evita "porta já em uso" após restart rápido)

**Desafios:**
- `accept()` bloqueante impedia Ctrl+C funcional - resolvido com `select()` + timeout
- Porta 8080 ficava ocupada após encerramento - resolvido com `SO_REUSEADDR`
- Quebras de linha em textarea contam como 2 caracteres (`\r\n`) no body HTTP
___
**_Apr 28_** - Refatoração TrateRequest: Método Helper sendPage
**Componente:** Refatoração e reutilização de código

**Resumo Técnico:**
- Criado método privado `sendPage()` em `TrateRequest` para reutilizar lógica de servir arquivos HTML
- Movido código de abertura/leitura/envio de arquivo de `ifGet()` para `sendPage()`
- `sendPage()` recebe `file_path` e `status_header` como parâmetros
- Permite servir arquivos com diferentes códigos HTTP (200 OK, 404 Not Found, 405 Method Not Allowed)
- Preparado para uso futuro com páginas de erro customizadas
- Corrigido links em `index.html`: `contatos.html` → `contacts.html`

**Testes Realizados:**
- Teste 1: `GET /` → serve `index.html` corretamente usando `sendPage()`
- Compilação e funcionamento validados

**Decisões de Arquitetura:**
- Separação de lógica: `sendPage()` cuida de I/O de arquivo, métodos HTTP decidem qual arquivo/status
- `sendPage()` privado porque é helper interno da classe
- Comentários adicionados indicando uso futuro com páginas de erro (`/error/404.html`, `/error/405.html`)

**Desafios:**
- Correção de assinatura de método (const reference vs value parameter)
- `sendPage()` precisa ser método de classe para acessar `_client_fd`
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
**_Apr 24_** - Merge user1 && user2.
___
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
