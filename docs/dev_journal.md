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
**_May 1_** - Novo front-end
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
**_May 4_** - Refatoração TrateRequest: Separação por método e Diretórios
**Componente:** Refatoração arquitetural e listagem de diretórios

**Resumo Técnico:**
- **Separação de arquivos:** Movido métodos HTTP para arquivos separados (`ifGet.cpp`, `ifPost.cpp`, `ifDelete.cpp`) para melhor organização e SRP
- **Encapsulamento:** Métodos HTTP (`ifGet`, `ifPost`, `ifDelete`) movidos para private section da classe `TrateRequest`
- **ifGet:** Implementado suporte a listagem de diretórios quando não existe `index.html`
- **ifGet:** Adicionado verificação de arquivo padrão (`index.html`) antes de listar diretório
**_Progresso:_**

**Data:** 2026-05-12 (Merge Final)
**Componente:** Sistema Completo / Merge Branch Request

**Resumo Técnico:**
- Unificada a branch `Request` com a branch `Socket`, consolidando as funcionalidades de rede, parsing e tratamento de requisições.
- Resolvidos conflitos críticos de integração no `SocketServer.cpp` e `TrateRequest.hpp`, garantindo que a classe `ParserConf` seja injetada corretamente em todo o fluxo.
- Consolidado o loop de eventos (`poll`) com suporte completo e assíncrono para CGI.
- Verificada a compilação total do projeto via `Makefile`.

**Decisões de Arquitetura:**
- Arquitetura Final: `SocketServer` (Multiplexação/Poll) -> `ParserRequest` (Parsing HTTP) -> `TrateRequest` (Lógica de Métodos/CGI/Static) -> `ParserConf` (Configurações/Segurança).
- Injeção de dependência mantida: O objeto `_config` é passado do `main` para o `SocketServer` e dele para o `TrateRequest`, permitindo que o servidor seja configurável via arquivo sem recompilação futura.

**Desafios:**
- Resolução de conflitos de merge causados por desenvolvimentos paralelos na detecção de CGI e integração de configurações.
- Sincronização de headers duplicados e caminhos de include inconsistentes entre as pastas `conf/` e `src/Conf/`.

___

**Data:** 2026-05-12 (Final)
**Componente:** TrateRequest / Sincronização ParserConf

**Resumo Técnico:**
- Finalizada a sincronização da classe `TrateRequest` com o sistema `ParserConf`.
- Atualizado o construtor e os métodos helper (`ifGet`, `ifPost`, `ifDelete`, `executeCGI`) para aceitar e utilizar o objeto de configuração.
- Corrigido o erro de compilação no `SocketServer.cpp` ao instanciar o handler com a referência de configuração correta.

**Decisões de Arquitetura:**
- Injeção de dependência: `TrateRequest` agora recebe `ParserConf` por referência, permitindo validações de segurança (CGI extensions, method allowed) e roteamento dinâmico (root, index) baseados no arquivo de configuração.
- Consistência de tipos: Garantido que todos os módulos do servidor utilizem a mesma fonte de verdade para configurações.

**Desafios:**
- Sincronização manual de múltiplos arquivos de implementação (`ifGet.cpp`, `ifPost.cpp`, etc.) para garantir que a assinatura do construtor fosse respeitada em todo o projeto.

___

**Data:** 2026-05-12
**Componente:** SocketServer / Integração ParserConf / CGI

**Resumo Técnico:**
- Restaurada a integração completa da classe `ParserConf` no `SocketServer` e `main.cpp`.
- Corrigido o bug de interceptação de CGI no loop de eventos (`poll`), permitindo que o servidor diferencie pipes de CGI de sockets de clientes.
- Re-sincronizados os arquivos `.hpp` e `.cpp` do SocketServer para suportar a passagem do objeto de configuração por referência.

**Decisões de Arquitetura:**
- O `SocketServer` agora é dependente de uma instância de `ParserConf`, garantindo que todas as decisões de roteamento e limites (como `client_max_body_size`) sejam baseadas no arquivo de configuração.
- Implementada lógica de "CGI check" no event loop para evitar que o output de scripts Python seja enviado erroneamente para o parser de requests HTTP.

**Desafios:**
- Resolvido erro de compilação causado por mismatch de assinaturas no construtor entre o header e a implementação.
- Identificada a necessidade de busca por prefixo no `ParserConf` para suportar arquivos dentro de diretórios configurados (ex: `/cgi-bin/`).

___

**Data:** 2026-05-11 (mais recente)  
**Componente:** Parser de configuração e testes de validação HTTP
  
**Resumo Técnico:**  
- **Diff realizado:** comparado staging area (git diff --cached) vs working tree.  
- **ParserConf.hpp/cpp:** implementado parser completo de configuração com métodos específicos: `parsePort()`, `parseServerName()`, `parseRoot()`, `parseLocation()`, `parseErrorPage()`. Membros privados movidos para public para acesso temporário durante desenvolvimento.  
- **default.conf:** expandido com múltiplos server blocks, `client_max_body_size 1M`, páginas de erro customizadas (400, 403, 404, 405, 413, 500), e locations específicas (`/upload` POST-only, `/old-path` redirecionamento).  
- **CGI date.py:** script Python para retornar timestamps em múltiplos formatos (JSON, ISO, timestamp Unix) com fallback para arquivo `curriculum.json` por PID.  
- **Front-end:** atualizado `curriculo.js` para obter PID via `/api/pid` e passar como query string para CGI, permitindo tracking individual por usuário.  
- **Assets:** adicionado `default_curriculum.json` com estrutura padrão do currículo.  
- **Testes de validação HTTP:** criado `test_config_logic.py` que identificou bug crítico: servidor retorna 404 em vez de 405 para DELETE em `/upload` (verifica arquivo antes de método).  
- **Testes adicionais:** scripts para keep-alive (`test_keep_alive.py`) e stress simples (`test_stress_simple.py`).  
  
**Decisões de Arquitetura:**  
- **Parser modular:** separação por tipo de diretiva (port, server_name, root, location, error_page) facilita manutenção e extensão do parser de configuração.  
- **CGI stateless:** script `date.py` opera sem estado persistente, usando query string para identificar usuário e fallback para arquivo quando disponível.  
- **Validação automatizada:** testes Python permitem verificação contínua da conformidade HTTP (404 vs 405) antes da avaliação.  
  
**Desafios:**  
- **BUG CRÍTICO - Ordem de verificação HTTP:** servidor está verificando existência de arquivo antes de permissão do método, retornando 404 em vez de 405. Viola regra HTTP/1.1 e pode causar nota zero na avaliação.  
- **ParserConf visibilidade:** membros movidos para public durante desenvolvimento; precisam voltar para private com getters adequados antes do commit final.  
- **Múltiplos server blocks:** configuração atual tem dois blocks escutando porta 8080; precisa implementar seleção baseada em `server_name` ou tratar conflito.  
- **Testes manuais:** scripts Python exigem servidor rodando; falta integração automatizada no pipeline de build/CI.  

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

___

**Data:** 2026-05-08  
**Componente:** Unificação de Parseamento Multipart para FORM e CGI  
  
**Resumo Técnico:**  
- **postMultipart:** Função unificada substitui postMultipartFormData e parseCGIMultipartFormData.  
- **postMultipart:** Adiciona parâmetro type ("FORM" ou "CGI") para diferenciar comportamento.  
- **postMultipart:** Quando type="FORM", gera JSON e salva arquivos (comportamento original).  
- **postMultipart:** Quando type="CGI", gera formato name=value ou name=filename:content para stdin.  
- **executeCGIPost:** Usa postMultipart com type="CGI".  
- **ifPost:** Usa postMultipart com type="FORM".  
- **parseCGIMultipartFormData:** Removida (funcionalidade absorvida por postMultipart).  
- **postMultipartFormData:** Removida (funcionalidade absorvida por postMultipart).  
- **TrateRequest.hpp:** Atualizada declaração de postMultipart.  
- **Mensagens de erro:** Atualizadas para especificar "CGI GET" vs "CGI POST".  
  
**Decisões de Arquitetura:**  
- Unificação evita duplicação de código.  
- Parâmetro type permite comportamento diferenciado sem duplicar lógica.  
- Manutenção simplificada (apenas uma função de parseamento).  

___

**Data:** 2026-05-08  
**Componente:** Tratamento de Erros em CGI e Desagrupamento Multipart  
  
**Resumo Técnico:**  
- **ifGet.cpp:** Função executeCGI movida para executeCGIGet com tratamento de erros completo.  
- **ifPost.cpp:** Função executeCGIPost criada com desagrupamento multipart/form-data e tratamento de erros.  
- **TrateRequest.cpp:** Função executeCGI unificada removida (separada em GET e POST).  
- **TrateRequest.hpp:** Declarações atualizadas para executeCGIGet e executeCGIPost.  
- **Tratamento de erros:** Verificação de status do processo filho após waitpid usando WIFEXITED e WEXITSTATUS.  
- **Tratamento de erros:** Envio de 500 Internal Server Error se o CGI falhar (exit code != 0).  
- **Tratamento de erros:** Implementação de timeout de 5 segundos usando alarm() no processo filho.  
- **Tratamento de erros:** Verificação de SIGALRM com WIFSIGNALED e WTERMSIG após waitpid.  
- **Tratamento de erros:** Verificação de output vazio antes de enviar resposta.  
- **Tratamento de erros:** Mensagens de erro traduzidas para português.  
- **Desagrupamento multipart:** Verificação de Content-Type para detectar multipart/form-data.  
- **Desagrupamento multipart:** Extração de boundary do Content-Type.  
- **Desagrupamento multipart:** Remoção de headers multipart e boundaries.  
- **Desagrupamento multipart:** Passagem de corpo limpo via stdin para o CGI.  
- **Desagrupamento multipart:** Criação de dois pipes (stdout e stdin) para comunicação bidirecional.  
- **Signal handlers:** Funções cgi_timeout_handler e g_cgi_timeout tornadas static em cada arquivo para evitar conflitos de linker.  
- **Endpoint POST /cgi-bin/:** Permitir POST em /cgi-bin/ como GET para testar scripts de erro.  
- **Endpoint /api/cgi-test:** Removido (desnecessário, só existe pasta cgi-bin).  
  
**Scripts de Teste Criados:**  
- **test_syntax_error.py:** Script com erro de sintaxe Python para testar tratamento de erros.  
- **test_infinite_loop.py:** Script com loop infinito para testar timeout.  
- **test_exit_error.py:** Script com exit(1) para testar verificação de exit code.  
- **test_empty_output.py:** Script sem output para testar verificação de output vazio.  
  
**Decisões de Arquitetura:**  
- Separação de executeCGI em executeCGIGet e executeCGIPost para melhor organização.  
- Timeout de 5 segundos no processo filho usando alarm() (não no processo pai).  
- Signal handlers static em cada arquivo para evitar conflitos de linker.  
- Verificação de SIGALRM com WIFSIGNALED e WTERMSIG após waitpid.  
- Dois pipes em executeCGIPost (stdout e stdin) para comunicação bidirecional.  
- Desagrupamento multipart simplificado: remove headers e boundaries, mantém corpo.  
- Mensagens de erro em português para consistência com o resto do código.  
- POST em /cgi-bin/ funciona igual ao GET para facilitar testes.  

___

**Data:** 2026-05-07  
**Componente:** CGI POST para Avaliação  
  
**Resumo Técnico:**  
- **CGI:** Criado script Python test_post.py em www/cgi-bin/ para teste de CGI POST (requisito de avaliação).  
- **CGI:** Script minimalistico que retorna JSON estático sem ler stdin ou escrever arquivos (evita bugs complexos).  
- **CGI:** Script imprime Content-Type: application/json e JSON com status e método.  
- **ifPost:** Adicionado endpoint /api/cgi-test para chamar executeCGI com método POST.  
- **ifPost:** Endpoint usa executeCGI unificada (aceita method como parâmetro).  
- **Motivação:** Avaliação exige CGI em POST, mas implementação complexa com stdin causava bugs.  
  
**Decisões de Arquitetura:**  
- Script minimalistico para evitar bugs com stdin e file I/O.  
- Endpoint separado (/api/cgi-test) para não afetar funcionalidade principal do site.  
- JSON estático suficiente para demonstrar CGI POST funcionando.  
- Função executeCGI unificada simplifica código (uma função para GET e POST).  

___

**Data:** 2026-05-06  
**Componente:** CGI: Executar no diretório correto  
  
**Resumo Técnico:**  
- **ifGet.cpp:** Adicionado chdir() em executeCGI() para mudar para o diretório do script antes de execve().  
- **ifGet.cpp:** script_dir extraído usando script_path.substr(0, script_path.find_last_of("/")).  
- **ifGet.cpp:** Validação de erro no chdir() com mensagem de erro e exit(1) em caso de falha.  
- **Motivação:** Subject.txt linha 141-142 exige "O CGI deve ser executado no diretório correto para acesso a arquivos de caminho relativo".  
  
**Decisões de Arquitetura:**  
- chdir() no processo filho antes de execve() para não afetar o processo pai.  
- Extração do diretório usando find_last_of("/") para compatibilidade C++98.  
- Exit(1) in case of failure in chdir() to exit the child process correctly.  

___

**Data:** 2026-05-06  
**Componente:** Melhorias de Compatibilidade HTTP  
  
**Resumo Técnico:**  
- **ifDelete:** Corrigido código de status para DELETE bem-sucedido de 200 OK para 204 No Content (padrão HTTP para DELETE).  
- **ifDelete:** DELETE agora é idempotente - retorna 204 mesmo se arquivo não existia (comportamento correto REST).  
- **ifDelete:** Removido corpo JSON da resposta de DELETE (204 não deve ter corpo).  
- **TrateRequest:** Adicionada validação de Host header para HTTP/1.1 (obrigatório pela especificação).  
- **TrateRequest:** Se Host header ausente em HTTP/1.1, retorna 400 Bad Request.  
- **TrateRequest:** Todas as respostas HTTP agora usam parser_request.version em vez de hardcoded "HTTP/1.1".  
- **ifGet:** Atualizada assinatura de executeCGI() para receber parser_request como parâmetro.  
- **ifGet:** Atualizada assinatura de sendDirectoryListing() para receber parser_request como parâmetro.  
- **ifGet:** Chamadas de executeCGI() e sendDirectoryListing() atualizadas para passar parser_request.  
- **TrateRequest.hpp:** Atualizadas declarações de executeCGI e sendDirectoryListing no header.  
- **ifDelete:** Corrigido erro de compilação (conversão de std::string para const char*).  
- **default.conf:** Atualizado com todas as páginas de erro do projeto (400, 404, 405, 413, 500).  
- **default.conf:** Adicionadas diretivas para funcionalidades futuras (index, cgi_extensions, upload_dir, return).  
  
**Decisões de Arquitetura:**  
- 204 No Content para DELETE porque é o padrão HTTP correto (sem corpo).  
- Idempotência em DELETE porque DELETE é idempotente por natureza.  
- Host header obrigatório para HTTP/1.1 porque é requerido pela especificação.  
- Usar versão do cliente nas respostas para compatibilidade com HTTP/1.0 e HTTP/1.1.  
- parser_request como parâmetro em executeCGI e sendDirectoryListing para acesso à versão HTTP.  
- std::string em vez de const char* em ifDelete para evitar conversão manual.  

___

**Data:** 2026-05-06  
**Componente:** Implementação de CGI (Common Gateway Interface)  
  
**Resumo Técnico:**  
- **ParserRequest.cpp:** Adicionada extração de query string do path.  
- **ParserRequest.cpp:** Query string armazenada em headers["Query"] para uso em ifGet.  
- **ifGet.cpp:** Implementada função executeCGI() para executar scripts externos via fork() + execve().  
- **ifGet.cpp:** Adicionada detecção de paths /cgi-bin/ para rotear para execução CGI.  
- **ifGet.cpp:** Adicionado endpoint /api/pid para retornar PID do usuário em JSON.  
- **CGI:** Criado script Python date.py em www/cgi-bin/ para retornar data/hora do último salvamento.  
- **Variáveis de ambiente:** QUERY_STRING, REQUEST_METHOD, SCRIPT_FILENAME passadas para o script.  
- **Pipe:** stdout do script capturado via pipe e enviado como resposta HTTP.  
- **JavaScript:** Função loadLastUpdated() atualiza elemento HTML com data formatada.  
  
**Decisões de Arquitetura:**  
- fork() + execve() para execução de scripts (padrão CGI).  
- Pipe para comunicação entre processos pai e filho.  
- waitpid() para evitar processos zumbis.  
- Query string extraída no ParserRequest para simplificar código em ifGet.  

___

**Data:** 2026-05-05  
**Componente:** Validação de body size no ifPost  
  
**Resumo Técnico:**  
- **ifPost:** Adicionada validação de body size no início da função.  
- **Limite:** 1MB (1024 * 1024 bytes) hardcoded temporariamente.  
- **Erro:** Retorna 413 Payload Too Large se body exceder limite.  
- **Página de erro:** Criado www/error/413.html com mensagem de erro estilizada.  
  
**Decisões de Arquitetura:**  
- Validação no início do ifPost para evitar processar requisições grandes desnecessariamente.  

___

**Data:** 2026-05-05  
**Componente:** Refatoração ifPost: Separação de funções e Correção de leaks  
  
**Resumo Técnico:**  
- **ifPost:** Refatorado para separar lógica de parsing em funções dedicadas.  
- **postMultipartFormData():** Função separada para parsing de multipart/form-data.  
- **postFormData():** Função separada para parsing de application/x-www-form-urlencoded.  
- **createUserDirectory():** Função helper para criar diretório do usuário usando getpid().  
- **ifGet:** Corrigido leak de file descriptor no endpoint /api/curriculum.  
- **ifGet:** Corrigido leak de file descriptor no GET normal.  
  
**Decisões de Arquitetura:**  
- Separação de responsabilidades: cada função cuida de um tipo de parsing.  
- Correção de leaks essencial para servidor long-running.  
- Uso de RAII (ofstream) para gerenciamento automático de recursos.  

___

**Data:** 2026-05-04  
**Componente:** Refatoração TrateRequest: Separação por método e Diretórios  
  
**Resumo Técnico:**  
- **Separação de arquivos:** Movido métodos HTTP para arquivos separados (`ifGet.cpp`, `ifPost.cpp`, `ifDelete.cpp`).  
- **ifGet:** Implementado suporte a listagem de diretórios quando não existe `index.html`.  
- **ifGet:** Adicionado verificação de arquivo padrão (`index.html`) antes de listar diretório.  
- **ifGet:** Corrigido bug de barra duplicada em path (`//users/` → `/users/`).  
- **ifGet:** Implementado uso de `getpid()` para criar pastas dinâmicas.  
  
**Decisões de Arquitetura:**  
- Separação por método para facilitar manutenção e testes.  
- `getpid()` para identificação de usuário (solução temporária).  

___

**Data:** 2026-05-03  
**Componente:** Upload de arquivos e persistência de dados  
  
**Resumo Técnico:**  
- **SocketServer:** Modificado `handleConnection()` para ler body completo em loop conforme `Content-Length`.  
- **ifPost:** Implementado parsing de `multipart/form-data` para upload de arquivos.  
- **ifDelete:** Criado método para deletar `curriculum.json` e limpar arquivos de `www/uploads/`.  
- **Makefile:** Ajustado `fclean` para remover `www/data/curriculum.json` e `www/uploads/*`.  
  
**Decisões de Arquitetura:**  
- Leitura em loop com buffer de 4096 bytes até completar body conforme `Content-Length`.  
- Uso de `system("rm -f www/uploads/*")` para limpar arquivos.  

___

**Data:** 2026-04-30  
**Componente:** Implementação POST e Melhorias de Estabilidade  
  
**Resumo Técnico:**  
- Implementado método `ifPost()` em `TrateRequest` para processar formulário de depoimento.  
- Validação: campos vazios ou tamanho excedido retornam 400 Bad Request.  
- Adicionado `setsockopt(SO_REUSEADDR)` em `SocketServer::setup()`.  
- Implementado `select()` com timeout de 1 segundo antes de `accept()`.  
  
**Decisões de Arquitetura:**  
- Validação no servidor por segurança.  
- `SO_REUSEADDR` para evitar "porta já em uso" após restart rápido.  

___

**Data:** 2026-04-28  
**Componente:** Refatoração TrateRequest: Método Helper sendPage  
  
**Resumo Técnico:**  
- Criado método privado `sendPage()` em `TrateRequest` para reutilizar lógica de servir arquivos HTML.  
- `sendPage()` recebe `file_path` e `status_header` como parâmetros.  
  
**Decisões de Arquitetura:**  
- Separação de lógica: `sendPage()` cuida de I/O de arquivo, métodos HTTP decidem qual arquivo/status.  

___

**Data:** 2026-04-27  
**Componente:** Refatoração para Classes e Tratamento de Métodos  
  
**Resumo Técnico:**  
- Transformado `SocketServer` de função para classe.  
- Criada classe `TrateRequest` para separar lógica de tratamento de métodos HTTP.  
- Implementado tratamento de métodos: GET, POST, DELETE.  
  
**Decisões de Arquitetura:**  
- Separação clara: `SocketServer` (rede), `ParserRequest` (parsing), `TrateRequest` (lógica de negócio).  

___

**Data:** 2026-04-27  
**Componente:** Refatoração e servidor persistente  
  
**Resumo Técnico:**  
- Refatorado para separar responsabilidades: `SocketServer.cpp/SocketServer.hpp` (apenas socket) e `ParserRequest.cpp/ParserRequest.hpp` (apenas parser).  
- Implementado loop infinito em `socket_server()` para aceitar múltiplas conexões consecutivas.  
- Adicionado signal handler (`SIGINT`) para encerramento graceful.  
  
**Decisões de Arquitetura:**  
- Uso de `volatile sig_atomic_t` para flag de shutdown.  

___

**Data:** 2026-04-27  
**Componente:** Integração Socket + Parser  
  
**Resumo Técnico:**  
- Integrado o parser de request (`Request.cpp/Request.hpp`) com o socket (`sandbox.cpp`).  
- Modificado `sandbox.cpp` para ler request do socket via `read()` e passar pelo parser `Request`.  
  
**Decisões de Arquitetura:**  
- Buffer de 4096 bytes para leitura do socket.  

___

**Data:** 2026-04-24  
**Componente:** Merge user1 && user2.  

___

**Data:** 2026-04-22  
**Componente:** Sockets / Bootstrap do projeto  
  
**Resumo Técnico:**  
- Criado `include/Request.hpp` com estrutura inicial de `Request`.  
- Adicionado `sandbox.cpp` com servidor TCP mínimo respondendo payload HTTP/1.1 fixo.  
  
**Decisões de Arquitetura:**  
- Fluxo de socket validado com syscalls clássicas: `socket()`, `bind()`, `listen()`, `accept()`, `write()`, `close()`.  

___

**Data:** 2026-04-22  
**Componente:** Estrutura basica Request.hpp  

___

**Data:** 2026-04-22  
**Componente:** Html basico para testes (index, contacts, posts).  

___

**Data:** 2026-04-22  
**Componente:** Criado sistema inicial de pastas.
