**_Progresso:_**

**Data:** 2026-05-11 (mais recente)  
**Componente:** CGI não-bloqueante via poll() (working tree vs HEAD)
  
**Resumo Técnico:**  
- **Diff realizado:** comparado working tree (não commitado) vs `HEAD (f6e5d43)`.  
- **SocketServer.hpp:** adicionados 3 novos maps para CGI: `_cgi_pipe_to_client` (pipe_fd → client_fd), `_cgi_pipe_to_pid` (pipe_fd → pid), `_cgi_buffers` (pipe_fd → output acumulado).  
- **TrateRequest.hpp:** adicionados membros `_cgi_fd` (-1 se não tem CGI) e `_cgi_pid`, mais métodos `hasCGI()`, `getCGIFd()`, `getCGIPid()`.  
- **SocketServer.cpp handleClientData():** quando `handler.hasCGI()` retorna true, registra pipe_fd no `_poll_fds` com evento `POLLIN` e popula os maps de CGI. NÃO muda client_fd para `POLLOUT` ainda.  
- **Novo método handleCGIRead():** lê dados do pipe em buffer de 4096 bytes, acumula em `_cgi_buffers[pipe_fd]`. Quando `read()` retorna 0 (pipe fechado), remove pipe_fd do poll, faz `waitpid(pid, &status, WNOHANG)`, monta resposta e muda client_fd correspondente para `POLLOUT`.  
- **TrateRequest.cpp:** implementados getters simples para CGI (`hasCGI`, `getCGIFd`, `getCGIPid`).  
  
**Decisões de Arquitetura:**  
- **CGI como FD monitorado:** pipe do CGI agora é tratado como qualquer outro FD no poll loop, eliminando bloqueio de `waitpid()` e `read()` síncronos.  
- **Buffering incremental de CGI:** output do CGI é acumulado em `_cgi_buffers[pipe_fd]` até pipe fechar, permitindo respostas CGI maiores que buffer size.  
- **Mapeamento indireto:** maps permitem encontrar client_fd e pid a partir do pipe_fd quando o CGI termina, mantendo desacoplamento entre componentes.  
  
**Desafios:**  
- **Complexidade de gerenciamento:** 3 maps adicionais aumentam complexidade e risco de leaks de FD se cleanup falhar.  
- **Temporário de DEBUG:** `std::cout` em `handleClientWrite()` precisa ser removido antes de commit.  
- **Headers CGI:** montagem de resposta ainda hardcoded ("HTTP/1.1 200 OK\r\n" + output), precisando de parser de headers do CGI.  

___

**Data:** 2026-05-11 (anterior)  
**Componente:** Documentação e regras operacionais
  
**Resumo Técnico:**  
- **Diff realizado:** comparado `HEAD (f6e5d43)` vs `HEAD~1 (b025834)`.  
- **Mudança no prompt_AI.md:** adicionada regra explícita de "Cronologia Obrigatória" na seção de comportamento (linha 15).  
- **Objetivo da mudança:** reforçar que a documentação deve seguir ordem temporal real dos fatos, tanto em fluxos de execução quanto em entradas do dev_journal.  
- **Impacto:** alinha o comportamento do agente documentador com as regras já existentes no `.cursorrules` sobre ordem cronológica (seção 4.0 do prompt).  
  
**Decisões de Arquitetura:**  
- Manutenção de consistência entre `.cursorrules` e `prompt_AI.md` para evitar ambiguidade na documentação.  
- A regra já estava implícita no prompt (seção 4.0) e explícita no `.cursorrules` (seção 2.2); agora está duplicada para ênfase.  
  
**Desafios:**  
- Nenhum desafio técnico encontrado na mudança; é apenas ajuste de documentação.  
- A duplicação da regra pode gerar redundância, mas reforça a importância da cronologia.  

___

**Data:** 2026-05-08  
**Componente:** `TrateRequest` — validação `Host`, refino de `sendPage`, remoção de `system()` em POST/DELETE
  
**Resumo Técnico:**  
- **`TrateRequest::TrateRequest`:** antes de despachar por método, se `version == "HTTP/1.1"` e não existe header `Host`, monta resposta `400 Bad Request` via `sendPage("www/error/400.html", ...)` e aborta o fluxo (virtual hosting mínimo).  
- **`sendPage`:** falha de `open()` deixa de logar em `stderr`; responde `404` com `Content-Length: 0` e `Connection: keep-alive`. Montagem de headers com `std::stringstream`; corpo montado com `std::string(file_content, file_size)` após `read()` no buffer alocado com `st_size`.  
- **`createUserDirectory()` (`ifPost.cpp`):** substituído `system("mkdir -p ...")` por `mkdir(2)` em `www/users/user<pid>` e em `.../uploads` (modo `0777`). Inclusões `<sys/wait.h>` e `<sys/stat.h>` para suportar o novo fluxo.  
- **`ifDelete`:** removido `system("rm -rf .../uploads/*")`; abre `user_dir/uploads/` com `opendir`, percorre `readdir`, ignora `.`/`..`, apaga cada entrada com `std::remove`, `closedir`. O JSON principal continua com `std::remove`. Resposta `204 No Content` inalterada em intenção.  
- **`TrateRequest.hpp` / `prompt_AI.md`:** comentários no header alinhados ao modelo "resposta em string"; em `prompt_AI.md`, texto do §2 e §4.0 simplificado (referência ao `subject.md` retirada; regras do journal menos prescritivas no arquivo de persona).  
  
**Decisões de Arquitetura:**  
- **Conformidade HTTP/1.1:** exigência de `Host` no construtor centraliza o erro antes de GET/POST/DELETE.  
- **Menos shell:** `mkdir`/`unlink`/`opendir` em vez de `system()` reduz dependência do `/bin/sh` e alinha o código a syscalls auditáveis (42).  
  
**Desafios:**  
- **`sendPage`:** `read()` não verifica bytes lidos; para ficheiros normais costuma coincidir com `file_size`, mas o caminho não trata leitura parcial nem erro de leitura explicitamente.  
- **`ifDelete`:** limpeza de `uploads/` é plana (sem árvore recursiva); entradas que forem diretórios ou hierarquias aninhadas não são equivalentes a um `rm -rf` antigo — [RISCO] se o upload criar subpastas.  
- **`ifDelete.cpp`:** inclusão duplicada de `<cstdio>` e possível `<sys/wait.h>` não usado — limpar num passe seguinte para evitar warnings.  
- **`mkdir`:** não há verificação explícita de falha nem `umask`; falhas silenciosas podem aparecer mais tarde no POST.  

___

**Data:** 2026-05-08  
**Componente:** Auditoria de Avaliação (Gap Analysis) vs `SocketServer` e `TrateRequest`

**Resumo Técnico:**  

- Verificação do código contra as regras rígidas da Scale:  
  - O `SocketServer` não avalia `errno` após as syscalls de I/O (`recv()` / `send()`), cumprindo a regra que dá nota zero se desrespeitada.  
  - Erros e fechamento de conexões estão operacionais e removem o cliente do vetor do `poll()`.  

**Decisões de Arquitetura:**  
- [NÃO IMPLEMENTADO] I/O de arquivos e pipes CGI ainda ocorrem fora do loop `poll()`.  

**Desafios:**  
- [VIOLAÇÃO] Regra "Writing or reading ANY file descriptor without going through select() (or equivalent) is strictly FORBIDDEN". Os arquivos estáticos e o pipe do CGI (`ifGet.cpp`) usam `read()`/`open()` síncronos fora da multiplexação. Isso acarreta zero imediato na defesa. É imperativo arquitetar o mapeamento destes FDs para `_poll_fds`.  
- [BUG] O handler do CGI (`executeCGIGet`) utiliza `waitpid(pid, &status, 0)` bloqueante. Isso trava o event loop, matando a multiplexação. Exige refatoração para FDs não-bloqueantes.  

___

**Data:** 2026-05-07  
**Componente:** Integração `SocketServer(poll)` → `ParserRequest` → `TrateRequest` (último commit vs penúltimo)
  
**Resumo Técnico:**  
- **Diff realizado:** comparado `HEAD (c9b9230)` vs `HEAD~1 (cd0c7a4)`.  
- **Ponte de integração no `SocketServer`:** quando `_client_buffers[fd]` atinge `expected_total_size`, extrai `raw_request`, instancia `ParserRequest(raw_request)` e chama `TrateRequest(parsed_req)`; a resposta passa a ser obtida por `handler.getResponse()` e enfileirada em `_client_responses[fd]`, mantendo o switch `POLLIN → POLLOUT` do fluxo assíncrono.  
- **`TrateRequest` sem `write()` direto:** refatorado para **montar a resposta em `_response`** (string) e expor `getResponse()`. O `client_fd` saiu do construtor.  
- **`sendPage()` e binários:** leitura passou a usar `std::string(file_content, bytes_read_file)` (sem `'\0'`), evitando truncar/corromper conteúdo binário.  
- **Keep-alive:** headers gerados pelo `TrateRequest` foram alinhados para `Connection: keep-alive` para não derrubar o loop baseado em `poll`.  
- **GET expandido:** adicionados handlers em `src/TrateRequest/ifGet.cpp` com `/api/pid`, `/api/curriculum`, execução de `/cgi-bin/*` e directory listing **em memória** (sem arquivo temporário).  
- **POST/DELETE separados:** `src/TrateRequest/ifPost.cpp` implementa persistência de `curriculum.json` em `www/users/user<pid>/` e upload; `src/TrateRequest/ifDelete.cpp` remove JSON e limpa uploads, retornando `204`.  
- **Parser:** `ParserRequest` passou a separar query string do path, salvando em `headers["Query"]`.  
- **Assets `www`:** novo CGI `www/cgi-bin/date.py`, novos erros `400/413/500`, `www/default_curriculum.json`, e ajustes em `templates.html`/`curriculo.js`/`curriculo.css` para exibir "última edição" via `/api/pid` + CGI.  
  
**Decisões de Arquitetura:**  
- **Camada de aplicação desacoplada do FD:** `TrateRequest` agora é puro "builder" de resposta (string), compatível com write parcial/`POLLOUT` do `SocketServer`.  
- **Reuso do buffering incremental existente:** `SocketServer` continua sendo o componente responsável por completude do request via `Content-Length` + buffer por FD; `ParserRequest` só parseia quando o pacote está completo.  
  
**Desafios:**  
- **CGI ainda é bloqueante no modelo atual:** `waitpid(pid, 0)` e `read()` do pipe no handler travam o event loop em requests CGI. Para casar com `poll`, CGI precisa virar FDs monitorados + reap via `waitpid(WNOHANG)`.  
- **`system()` presente em POST/DELETE:** `mkdir -p` e `rm -rf` ainda são shell-outs. Isso conflita com robustez/segurança e pode bloquear; precisa migrar para syscalls (`mkdir`, `unlink`, `rmdir`, `opendir/readdir`).  
- **Semântica CGI/headers:** o CGI `date.py` imprime headers HTTP. O handler em `ifGet.cpp` também injeta status line/`Content-Length`; isso exige padronizar se o output do CGI é "body puro" ou "header+body", senão pode gerar resposta inválida.  

___

**Data:** 2026-05-07  
**Componente:** Reintegração branch `Request` (TrateRequest + assets `www`) e divergências de arquitetura
  
**Resumo Técnico:**  
- **Código trazido:** adicionados `src/TrateRequest/TrateRequest.cpp`, `src/TrateRequest/ifGet.cpp`, `src/TrateRequest/ifPost.cpp`, `src/TrateRequest/ifDelete.cpp`; `src/ParserRequest.cpp` foi alterado.  
- **TrateRequest (roteamento por método):** construtor valida `Host` em HTTP/1.1 e despacha `GET/POST/DELETE`, com fallback para `405`.  
- **GET (estático + API + diretório + CGI):**  
  - `/api/curriculum` lê `www/users/user<pid>/curriculum.json` e faz fallback em `www/default_curriculum.json`.  
  - `/api/pid` retorna JSON com `getpid()`.  
  - `/cgi-bin/*` executa script via CGI e retorna output.  
  - `opendir()` em path: tenta `index.html`, senão gera directory listing (HTML) e serve via arquivo temporário.  
- **POST:** `/api/curriculum` aceita `multipart/form-data` (boundary) e `application/x-www-form-urlencoded` (parser simples), grava `curriculum.json` em diretório por PID e redireciona `302 Found` para `Referer`. Rejeita body > 1MB com `413`.  
- **DELETE:** `/api/curriculum` remove JSON e limpa uploads do diretório por PID, retornando `204`.  
  
**Decisões de Arquitetura:**  
- **CGI:** uso de `pipe()` + `fork()` + `dup2(STDOUT_FILENO)` + `execve()` para capturar stdout do script no pai, seguido de `waitpid()` para sincronizar término do filho.  
- **Isolamento por "usuário":** path baseado em `getpid()` (`www/users/user<pid>/...`) para evitar colisão simples entre execuções no mesmo host, sem state global.  
- **Servir arquivos:** `open()` + `fstat()` + `read()` + `write()` com `Content-Length` baseado em `st_size`.  
  
**Desafios:**  
- **Divergência com o event loop non-blocking:** `waitpid()` no caminho de `GET`/CGI e `read()` do pipe em loop são operações potencialmente bloqueantes; para casar com `poll()`, o CGI deveria virar um conjunto de FDs (stdin/stdout pipes) monitorados no loop e o `waitpid(..., WNOHANG)` deveria ser usado para reap/retry sem travar o servidor.  
- **Uso de `system()` para `mkdir`/`rm`:** introduz dependência de shell, superfície de ataque e bloqueio; precisa ser substituído por syscalls (`mkdir(2)`, `unlink(2)`, `opendir/readdir` para limpeza) e integrado ao modelo de erro do servidor.  
- **Headers HTTP inconsistentes:** `sendPage()` assume que `status_header` já inclui quebras/formatting corretos; há chamadas com e sem `\r\n` embutido. Isso tende a gerar resposta malformada se não padronizar "status line + headers".  
- **ParserRequest é "split" ingênuo:** separa header/body por `\r\n\r\n` e não valida `Content-Length`/completude; isso conflita com a estratégia atual de buffering incremental por FD no servidor.  

___

**Data:** 2026-05-07  
**Componente:** Testes manuais (nc) + observabilidade de body parcial em `handleClientData`
  
**Resumo Técnico:**  
- **Teste manual:** adicionado `tests/test.sh` com um POST "lento" via `nc` (envia headers, dorme, depois envia body) para validar que o servidor espera o body até `Content-Length`.  
- **Log de progresso:** no caminho "ainda faltam bytes do body", o código imprime `Recebendo arquivo grande... (atual/esperado)` para acompanhar recebimento incremental durante uploads/requisições grandes.  
  
**Decisões de Arquitetura:**  
- O teste usa o padrão de "escrita fracionada" (headers primeiro, body depois) para simular a realidade de TCP. Ele exercita o buffer por FD e o critério `expected_total_size`.  
  
**Desafios:**  
- Esses logs verbosos podem poluir stdout no fluxo normal; no futuro, condicionar por flag de debug ou remover após estabilizar o parser/estado do request.  

___

**Data:** 2026-05-07  
**Componente:** SocketServer — parsing incremental de body via `Content-Length` (poll) + correção de build
  
**Resumo Técnico:**  
- **Parser (stub):** `handleClientData()` agora separa `header_end = find("\\r\\n\\r\\n")`, extrai `Content-Length` quando presente no header, calcula `expected_total_size = header_end + 4 + content_length`, e só considera o request "completo" quando `_client_buffers[fd].size() >= expected_total_size`.  
- **Buffering:** ao completar um request, usa `_client_buffers[fd].erase(0, expected_total_size)` (em vez de `clear()`) para preservar bytes remanescentes em caso de pipelining/back-to-back requests no mesmo FD.  
- **Resposta:** mantém o modelo de fila (`_client_responses[fd]`) + troca do `pollfd.events` para `POLLOUT`, e ao esvaziar o buffer de resposta retorna o FD para `POLLIN` (keep-alive).  
- **Build fix:** inclusão de `<cstdlib>` para expor `::atoi` usado na conversão de `Content-Length` (C++98/libc).  
  
**Decisões de Arquitetura:**  
- Parsing de request foi mantido "incremental" e acoplado ao buffer por FD (map `fd -> std::string`), evitando bloqueio: cada `recv()` só avança estado se o buffer acumulado já contém bytes suficientes.  
- Conversão numérica feita com `::atoi` por compatibilidade C++98; validação de input (não numérico/overflow) fica como etapa separada do parsing.  
  
**Desafios:**  
- `Content-Length` inválido/ausente exige política clara (400 vs esperar) e limites (max body). `atoi` não sinaliza erro.  
- O critério "request completo" ainda não cobre chunked transfer/pipelining completo; precisa de máquina de estados real para HTTP/1.1.  

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
- Política de "última atividade" por FD: só fecha inatividade prolongada no sentido de I/O processado, não reimplementa regra HTTP de `Keep-Alive` completa.  
- Listen isolado da varredura de timeout para não confundir socket de serviço com cliente.

**Desafios:**  
- Navegadores podem abrir conexão e demorar a mandar bytes; se 30 s for agressivo para o ambiente de teste, ajustar constante ou diferenciar idle pré-request vs pós-resposta.  

___

**Data:** 2026-05-04  
**Componente:** Resposta HTTP — `Content-Length` exato, `<sstream>`

**Resumo Técnico:**  
- Stub em `handleClientData`: corpo em `std::string html_body`; cabeçalho montado com `std::ostringstream` (`Content-Type`, `Connection: close`, `Content-Length: ` + `html_body.length()`, depois `\r\n\r\n` e o corpo). C++98 (`<sstream>`).  
- **Bug:** header fixo prometia 47 octetos e o HTML tinha 46; navegadores (ex.: Brave) bloqueiam na UI até receber todos os bytes prometidos ou abortam se a conexão cair antes — sintoma "tela preta".  
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
- POST / body grande: acumular até `Content-Length`.  
- `accept()` não bloqueante: `EAGAIN` vs erro fatal.  

___

**Data:** 2026-05-03  
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

**Data:** 2026-05-01  
**Componente:** Novo front-end  

___

**Data:** 2026-04-30  
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

**Data:** 2026-04-28  
**Componente:** Refatoração TrateRequest: Método Helper sendPage

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

**Data:** 2026-04-27 (noite)  
**Componente:** Refatoração para Classes e Tratamento de Métodos

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
- Separação de lógica: `SocketServer` (rede), `ParserRequest` (parsing), `TrateRequest` (lógica de negócio)  
- Classe `SocketServer` com métodos `setup()`, `handleConnection()`, `run()`  
- `TrateRequest` decide ação baseado no método no construtor  
- Uso de `new/delete` para buffer de arquivo (C++98, sem smart pointers)  

**Desafios:**  
- Correção de includes em `SocketServer.hpp` (removidos includes de implementação)  
- Arquivos renomeados para PascalCase (`SocketServer.cpp`, `ParserRequest.cpp`, `TrateRequest.cpp`)  

___

**Data:** 2026-04-27 (tarde)  
**Componente:** Refatoração e Loop de Conexões

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

**Data:** 2026-04-27  
**Componente:** Integração Socket + Parser

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

**Data:** 2026-04-24  
**Componente:** Merge user1 && user2.  

___

**Data:** 2026-04-22  
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

**Data:** 2026-04-22  
**Componente:** Estrutura basica Request.hpp  

___

**Data:** 2026-04-22  
**Componente:** Html basico para testes (index, contacts, posts).  

___

**Data:** 2026-04-22  
**Componente:** Criado sistema inicial de pastas.
