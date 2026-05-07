**_Progresso:_**

**Data:** 2026-05-07  
**Componente:** Testes manuais (nc) + observabilidade de body parcial em `handleClientData`
  
**Resumo Técnico:**  
- **Teste manual:** adicionado `tests/test.sh` com um POST “lento” via `nc` (envia headers, dorme, depois envia body) para validar que o servidor espera o body até `Content-Length`.  
- **Log de progresso:** no caminho “ainda faltam bytes do body”, o código imprime `Recebendo arquivo grande... (atual/esperado)` para acompanhar recebimento incremental durante uploads/requisições grandes.  
  
**Decisões de Arquitetura:**  
- O teste usa o padrão de “escrita fracionada” (headers primeiro, body depois) para simular a realidade de TCP. Ele exercita o buffer por FD e o critério `expected_total_size`.  
  
**Desafios:**  
- Esses logs verbosos podem poluir stdout no fluxo normal; no futuro, condicionar por flag de debug ou remover após estabilizar o parser/estado do request.  

___
**Data:** 2026-05-07  
**Componente:** SocketServer — parsing incremental de body via `Content-Length` (poll) + correção de build
  
**Resumo Técnico:**  
- **Parser (stub):** `handleClientData()` agora separa `header_end = find("\\r\\n\\r\\n")`, extrai `Content-Length` quando presente no header, calcula `expected_total_size = header_end + 4 + content_length`, e só considera o request “completo” quando `_client_buffers[fd].size() >= expected_total_size`.  
- **Buffering:** ao completar um request, usa `_client_buffers[fd].erase(0, expected_total_size)` (em vez de `clear()`) para preservar bytes remanescentes em caso de pipelining/back-to-back requests no mesmo FD.  
- **Resposta:** mantém o modelo de fila (`_client_responses[fd]`) + troca do `pollfd.events` para `POLLOUT`, e ao esvaziar o buffer de resposta retorna o FD para `POLLIN` (keep-alive).  
- **Build fix:** inclusão de `<cstdlib>` para expor `::atoi` usado na conversão de `Content-Length` (C++98/libc).  
  
**Decisões de Arquitetura:**  
- Parsing de request foi mantido “incremental” e acoplado ao buffer por FD (map `fd -> std::string`), evitando bloqueio: cada `recv()` só avança estado se o buffer acumulado já contém bytes suficientes.  
- Conversão numérica feita com `::atoi` por compatibilidade C++98; validação de input (não numérico/overflow) fica como etapa separada do parsing.  
  
**Desafios:**  
- `Content-Length` inválido/ausente exige política clara (400 vs esperar) e limites (max body). `atoi` não sinaliza erro.  
- O critério “request completo” ainda não cobre chunked transfer/pipelining completo; precisa de máquina de estados real para HTTP/1.1.  

___
**Data:** 2026-05-06  
**Componente:** SocketServer — múltiplos sockets de listen (N portas) e roteamento `POLLIN` server/cliente
  
**Resumo Técnico:**  
- **Listen multiporta:** `setup()` passou a criar `socket()`/`bind()`/`listen()` por porta em `_ports` e registrar cada FD em `_server_fds` + inserir um `pollfd` correspondente em `_poll_fds` (evento `POLLIN`).  
- **Dispatch no loop:** `run()` usa `isServerSocket(fd)` para distinguir FD de servidor de FD de cliente; ao receber `POLLIN` em server FD, chama `acceptNewConnection(server_fd)` com o FD correto (não assume FD único).  
- **Timeouts:** `checkTimeouts()` passou a varrer todos os `_poll_fds`, ignorando server sockets via `isServerSocket(fd)`, e só derruba cliente se existir entrada em `_client_last_activity` e exceder 30s.  
- **Repo/higiene:** houve remoção de arquivos sob `conf/` e remoção de `docs/subject.md`; `prompt_AI.md` foi ajustado para referenciar o subject. Artefatos (`objs/`, binário) permanecem como arquivos locais.  
  
**Decisões de Arquitetura:**  
- `poll()` unifica listen sockets e clientes num único vetor; a distinção server/cliente é lógica (`_server_fds`), mantendo o loop single-thread e non-blocking.  
  
**Desafios:**  
- Como há múltiplas portas, logs/config precisam mapear porta↔FD para debug e roteamento futuro por server block do `.conf`.  

___
**Data:** 2026-05-04  
**Componente:** SocketServer — timeout de inatividade (`checkTimeouts`), ajustes de `poll`

**Resumo Técnico:**  
- **Build:** em `run()`, a comparação do FD de listen com o socket do servidor usa `server_fd` (membro real da classe); `checkTimeouts()` declarado em `SocketServer.hpp` e definido em `SocketServer.cpp` (corrige erro de escopo / protótipo ausente).  
- **`_client_last_activity`:** `std::map<int, time_t>` preenchido no `accept`; atualizado após `recv` com dados e após `send` parcial que avança a resposta.  
- **Loop:** `poll(..., 2000)` — até 2 s de espera para acordar o loop mesmo sem eventos de I/O, permitindo checar timeouts com regularidade. Depois de tratar `POLLIN`/`POLLOUT` (se `poll_count > 0`), chama-se sempre `checkTimeouts()`.  
- **`checkTimeouts()`:** percorre apenas clientes (índices `i >= 1` em `_poll_fds`, índice 0 = listen); se `difftime(now, _client_last_activity[fd]) > 30` segundos, log `[TIMEOUT] Cliente fantasma detectado e derrubado` e `closeConnection`.  
- **Teste no terminal:** cliente FD 4 conectou e foi encerrado por esse timeout — coerente com conexão TCP sem tráfego HTTP (ou sem renovação de atividade) além de 30 s, ou com espera antes de enviar o request.

**Decisões de Arquitetura:**  
- Política de “última atividade” por FD: só fecha inatividade prolongada no sentido de I/O processado, não reimplementa regra HTTP de `Keep-Alive` completa.  
- Listen isolado da varredura de timeout para não confundir socket de serviço com cliente.

**Desafios:**  
- Navegadores podem abrir conexão e demorar a mandar bytes; se 30 s for agressivo para o ambiente de teste, ajustar constante ou diferenciar idle pré-request vs pós-resposta.

___
**Data:** 2026-05-04  
**Componente:** Resposta HTTP — `Content-Length` exato, `<sstream>`

**Resumo Técnico:**  
- Stub em `handleClientData`: corpo em `std::string html_body`; cabeçalho montado com `std::ostringstream` (`Content-Type`, `Connection: close`, `Content-Length: ` + `html_body.length()`, depois `\r\n\r\n` e o corpo). C++98 (`<sstream>`).  
- **Bug:** header fixo prometia 47 octetos e o HTML tinha 46; navegadores (ex.: Brave) bloqueiam na UI até receber todos os bytes prometidos ou abortam se a conexão cair antes — sintoma “tela preta”.  
- **Correção:** o valor de `Content-Length` passou a ser derivado de `html_body.length()`, alinhado ao payload real.

**Decisões de Arquitetura:**  
- Contrato HTTP: `Content-Length` deve ser o comprimento em octetos do body **tal como enviado** após o `\r\n\r\n`; calcular a partir do mesmo buffer/string que compõe o body elimina divergência manual.

**Desafios:**  
- Ao reintegrar `TrateRequest`, montar o body primeiro e só então emitir headers com o comprimento correto (mesmo padrão).

___
**Data:** 2026-05-04  
**Componente:** SocketServer — `poll()`, I/O não bloqueante, `POLLOUT`

**Resumo Técnico:**  
- **Pipeline (runtime):** `poll()` sobre `std::vector<struct pollfd>`; `fcntl(..., O_NONBLOCK)` no listen e nos clientes; `recv()` → `_client_buffers` até `\r\n\r\n`; resposta enfileirada em `_client_responses`, `events` `POLLIN` → `POLLOUT`; `handleClientWrite()` com `send()` parcial; `closeConnection()`; destrutor fecha FDs em `_poll_fds`.  
- **Integração:** `ParserRequest` / `TrateRequest` ainda não no caminho deste stub de teste.  
- **Repositório (eixo commits):** alvo `webserv`, README, merges parser/config, ajustes em `TrateRequest` / `www`.

**Decisões de Arquitetura:**  
- `poll` com timeout para não bloquear indefinidamente (checagem de `g_running`; hoje 2000 ms no código e uso conjunto com `checkTimeouts`).  
- Varredura reversa de `_poll_fds` ao fechar conexões; `POLLIN`/`POLLOUT` para writes parciais.

**Desafios:**  
- Reintegrar parser/tratamento na fila de resposta.  
- POST / body grande: acumular até `Content-Length` (ver **May 3** abaixo).  
- `accept()` não bloqueante: `EAGAIN` vs erro fatal.

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
