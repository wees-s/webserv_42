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
- **ifGet:** Corrigido bug de barra duplicada em path (`//users/` → `/users/`)
- **ifGet:** Implementado uso de `getpid()` para criar pastas dinâmicas (`www/users/user<PID>/`)
- **ifDelete:** Implementado uso de `getpid()` para identificar pasta do usuário
- **ifDelete:** Corrigido comando de limpeza de uploads (`rm -rf user_dir/uploads/*`)
- **ifPost:** Corrigido bug de foto não encontrada - upload_path e photoUrl agora usam `user_dir` dinâmico em vez de hardcoded `user1`
- **C++98:** Substituído `std::to_string` por `std::stringstream` para compatibilidade
- **C++98:** Substituído `path.back()` por `path[path.length() - 1]` para compatibilidade
- **Headers:** Adicionados includes necessários em cada arquivo (`unistd.h`, `dirent.h`, `cstdio`, `sstream`)

**Testes Realizados:**
- Teste 1: Listagem de diretório em `/users/` → mostra pastas de usuários ✓
- Teste 2: Upload de foto → salva em pasta correta e URL atualizada ✓
- Teste 3: Limpar dados → remove JSON e arquivos de uploads ✓
- Teste 4: Compilação C++98 sem erros ✓

**Decisões de Arquitetura:**
- Separação por método para facilitar manutenção e testes
- Métodos privados para encapsulamento (apenas construtor público)
- `getpid()` para identificação de usuário (solução temporária antes de sessão/cookies)
- `std::stringstream` em vez de `std::to_string` para C++98

**Desafios:**
- Compilação C++98 exigiu substituição de features C++11 (`to_string`, `back()`)
- Bug de foto não encontrada causado por mismatch entre user_dir hardcoded e dinâmico
- Path duplicando barra corrigido verificando primeiro caractere do path
___
**_May 5_** - Refatoração ifPost: Separação de funções e Correção de leaks
**Componente:** Refatoração arquitetural e correção de memory leaks

**Resumo Técnico:**
- **ifPost:** Refatorado para separar lógica de parsing em funções dedicadas
- **postMultipartFormData():** Função separada para parsing de multipart/form-data, recebe user_dir, content_type e parser_request
- **postFormData():** Função separada para parsing de application/x-www-form-urlencoded, recebe parser_request
- **createUserDirectory():** Função helper para criar diretório do usuário usando getpid()
- **ifGet:** Corrigido leak de file descriptor no endpoint /api/curriculum (close(file_fd) movido para else)
- **ifGet:** Corrigido leak de file descriptor no GET normal (close(file_fd) adicionado antes de sendPage)
- **ifGet:** Removida chamada redundante de open() quando arquivo não existe
- **Headers:** Atualizado TrateRequest.hpp para incluir parâmetros nas funções postFormData e postMultipartFormData

**Testes Realizados:**
- Teste 1: Upload de arquivo com multipart/form-data funciona ✓
- Teste 2: Parsing de form-urlencoded funciona ✓
- Teste 3: Criação de diretório dinâmico com getpid() funciona ✓
- Teste 4: Compilação sem erros ✓
- Teste 5: Verificação de memory leaks - todos file descriptors fechados ✓

**Decisões de Arquitetura:**
- Separação de responsabilidades: cada função cuida de um tipo de parsing
- Funções helper (createUserDirectory) para reutilização de código
- Correção de leaks essencial para servidor long-running
- Uso de RAII (ofstream) para gerenciamento automático de recursos

**Desafios:**
- Correção de assinatura de funções para incluir parâmetros necessários (parser_request)
- Identificação de leaks de file descriptor em código que usava sendPage (que reabre arquivo)
- Valgrind/verificação manual necessária para garantir fechamento de todos file descriptors
___
**_May 5 (tarde)_** - Validação de body size no ifPost
**Componente:** Validação de requisições POST

**Resumo Técnico:**
- **ifPost:** Adicionada validação de body size no início da função
- **Limite:** 1MB (1024 * 1024 bytes) hardcoded temporariamente
- **Erro:** Retorna 413 Payload Too Large se body exceder limite
- **Página de erro:** Criado www/error/413.html com mensagem de erro estilizada
- **Comentário:** Adicionado comentário indicando que o tamanho deve ser configurável via config

**Testes Realizados:**
- Validação funciona corretamente ✓

**Decisões de Arquitetura:**
- Validação no início do ifPost para evitar processar requisições grandes desnecessariamente
- Limite hardcoded temporariamente (deve ser configurável via arquivo de configuração conforme subject.txt)
___
**_May 6_** - Implementação de CGI (Common Gateway Interface)
**Componente:** Execução de scripts externos via HTTP

**Resumo Técnico:**
- **ParserRequest.cpp:** Adicionada extração de query string do path (ex: `/cgi-bin/date.py?124343` → path=`/cgi-bin/date.py`, Query=`124343`)
- **ParserRequest.cpp:** Query string armazenada em headers["Query"] para uso em ifGet
- **ifGet.cpp:** Implementada função executeCGI() para executar scripts externos via fork() + execve()
- **ifGet.cpp:** Adicionada detecção de paths /cgi-bin/ para rotear para execução CGI
- **ifGet.cpp:** Adicionado endpoint /api/pid para retornar PID do usuário em JSON
- **CGI:** Criado script Python date.py em www/cgi-bin/ para retornar data/hora do último salvamento
- **CGI:** Script usa python3 (shebang corrigido de python para python3)
- **CGI:** Script lê mtime do arquivo curriculum.json (www/users/user<PID>/curriculum.json)
- **CGI:** Se arquivo não existe, retorna hora atual como fallback
- **Variáveis de ambiente:** QUERY_STRING, REQUEST_METHOD, SCRIPT_FILENAME passadas para o script
- **Pipe:** stdout do script capturado via pipe e enviado como resposta HTTP
- **Front-end:** Adicionado elemento HTML em templates.html para exibir data de última atualização
- **CSS:** Estilo .last-updated (canto superior direito, cor branca)
- **JavaScript:** Função loadLastUpdated() chama /api/pid para obter PID do usuário
- **JavaScript:** loadLastUpdated() chama /cgi-bin/date.py?PID para obter mtime do arquivo
- **JavaScript:** loadLastUpdated() atualiza elemento HTML com data formatada

**Testes Realizados:**
- Teste 1: Execução direta de /cgi-bin/date.py retorna JSON correto ✓
- Teste 2: Variáveis de ambiente passadas corretamente ✓
- Teste 3: Stdout capturado e retornado como resposta ✓
- Teste 4: Query string extraída corretamente do path ✓
- Teste 5: /api/pid retorna PID do usuário corretamente ✓
- Teste 6: Data de última atualização exibida em templates.html ✓
- Teste 7: Sem arquivo curriculum.json: mostra hora atual ✓
- Teste 8: Com arquivo curriculum.json: mostra mtime do arquivo ✓

**Decisões de Arquitetura:**
- fork() + execve() para execução de scripts (padrão CGI)
- Pipe para comunicação entre processos pai e filho
- waitpid() para evitar processos zumbis
- Script Python simples sem dependências externas
- JSON como formato de resposta (fácil parse via JavaScript)
- Query string extraída no ParserRequest para simplificar código em ifGet
- Endpoint /api/pid necessário porque JavaScript não tem acesso ao PID do servidor
- Fallback para hora atual quando arquivo não existe (UX melhor que erro)

**Desafios:**
- python vs python3: shebang corrigido para python3
- Permissão de execução: chmod +x aplicado ao script
- Query string não estava sendo extraída: adicionada lógica no ParserRequest
- Content-Type duplicado: corrigido para não adicionar header duplicado
- JavaScript não tinha acesso ao PID: criado endpoint /api/pid
___
**_May 6 (tarde)_** - Melhorias de Compatibilidade HTTP
**Componente:** Códigos de status HTTP e validação de headers

**Resumo Técnico:**
- **ifDelete:** Corrigido código de status para DELETE bem-sucedido de 200 OK para 204 No Content (padrão HTTP para DELETE)
- **ifDelete:** DELETE agora é idempotente - retorna 204 mesmo se arquivo não existia (comportamento correto REST)
- **ifDelete:** Removido corpo JSON da resposta de DELETE (204 não deve ter corpo)
- **TrateRequest:** Adicionada validação de Host header para HTTP/1.1 (obrigatório pela especificação)
- **TrateRequest:** Se Host header ausente em HTTP/1.1, retorna 400 Bad Request
- **TrateRequest:** Todas as respostas HTTP agora usam parser_request.version em vez de hardcoded "HTTP/1.1"
- **ifGet:** Atualizada assinatura de executeCGI() para receber parser_request como parâmetro
- **ifGet:** Atualizada assinatura de sendDirectoryListing() para receber parser_request como parâmetro
- **ifGet:** Chamadas de executeCGI() e sendDirectoryListing() atualizadas para passar parser_request
- **TrateRequest.hpp:** Atualizadas declarações de executeCGI e sendDirectoryListing no header
- **ifDelete:** Corrigido erro de compilação (conversão de std::string para const char*)
- **default.conf:** Atualizado com todas as páginas de erro do projeto (400, 404, 405, 413, 500)
- **default.conf:** Adicionadas diretivas para funcionalidades futuras (index, cgi_extensions, upload_dir, return)

**Testes Realizados:**
- Teste 1: DELETE com arquivo existente → retorna 204 No Content ✓
- Teste 2: DELETE sem arquivo → retorna 204 No Content (idempotente) ✓
- Teste 3: HTTP/1.1 sem Host header → retorna 400 Bad Request ✓
- Teste 4: HTTP/1.0 sem Host header → aceito (não é obrigatório) ✓
- Teste 5: Respostas usam mesma versão HTTP que cliente ✓
- Teste 6: Compilação sem erros ✓

**Decisões de Arquitetura:**
- 204 No Content para DELETE porque é o padrão HTTP correto (sem corpo)
- Idempotência em DELETE porque DELETE é idempotente por natureza (deletar algo que não existe é sucesso)
- Host header obrigatório para HTTP/1.1 porque é requerido pela especificação (virtual hosting)
- Usar versão do cliente nas respostas para compatibilidade com HTTP/1.0 e HTTP/1.1
- parser_request como parâmetro em executeCGI e sendDirectoryListing para acesso à versão HTTP
- std::string em vez de const char* em ifDelete para evitar conversão manual

**Desafios:**
- parser_request não disponível em executeCGI e sendDirectoryListing: adicionado como parâmetro
- Conversão std::string para const char* em ifDelete: mudado para std::string + .c_str()
- Compilação após mudanças de assinatura: atualizadas todas as chamadas
___
**_May 6 (tarde 2)_** - CGI: Executar no diretório correto
**Componente:** Compatibilidade CGI com arquivos relativos

**Resumo Técnico:**
- **ifGet.cpp:** Adicionado chdir() em executeCGI() para mudar para o diretório do script antes de execve()
- **ifGet.cpp:** script_dir extraído usando script_path.substr(0, script_path.find_last_of("/"))
- **ifGet.cpp:** Validação de erro no chdir() com mensagem de erro e exit(1) em caso de falha
- **Motivação:** Subject.txt linha 141-142 exige "O CGI deve ser executado no diretório correto para acesso a arquivos de caminho relativo"

**Testes Realizados:**
- Compilação sem erros ✓

**Decisões de Arquitetura:**
- chdir() no processo filho antes de execve() para não afetar o processo pai
- Extração do diretório usando find_last_of("/") para compatibilidade C++98
- Exit(1) em caso de falha no chdir() para encerrar o processo filho corretamente

**Desafios:**
- Nenhum desafio encontrado na implementação
___