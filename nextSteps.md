# Próximos Passos — Claudio (Socket / Motor de Rede)

---

## Contexto

O `SocketServer` está funcional: `poll()` não-bloqueante, múltiplas portas, keep-alive, timeouts, integração com `ParserRequest` e `TrateRequest`. Os três bugs críticos que quebraram o site foram corrigidos (veja `ifPost`, `ifGet`, `TrateRequest`).

O que falta são as pontes entre as três camadas do trio:

```
[Wesley: ParserConf] ──→ [Claudio: SocketServer] ──→ [Companheira: TrateRequest]
```

---

## Prioridade 1 — Integrar o Config do Wesley no SocketServer

**Arquivo:** `src/main.cpp` + `src/SocketServer.cpp`

O `main.cpp` atual tem as portas hardcoded e o argumento de config comentado:

```cpp
// ATUAL — a corrigir:
ports.push_back(8080);
ports.push_back(8081);
...
SocketServer server(ports);
```

Quando o Wesley entregar a estrutura `ServerConfig`, o fluxo correto é:

```cpp
// FUTURO:
int main(int argc, char **argv) {
    if (argc != 2) { /* erro */ return 1; }

    ParserConf conf(argv[1]);
    // ler portas do conf → passar para SocketServer
    // ler client_max_body_size → passar para TrateRequest
    // ler error_pages → passar para TrateRequest
    // ler locations/methods → passar para TrateRequest
}
```

**O que você precisa do Wesley:**
- A estrutura `ServerConfig` pronta (ou pelo menos os getters de `port`, `client_max_body_size`, `error_pages`, `locations`)
- Confirmação de que `ParserConf` lida com múltiplos blocos `server {}` (uma porta por bloco)

**O que você precisa fazer:**
- Atualizar o construtor `SocketServer` para aceitar `ServerConfig` em vez de `vector<int>`
- Remover os `ports.push_back` hardcoded do `main.cpp`
- Descomentar e validar a leitura do `argv[1]`

---

## Prioridade 2 — Passar Config para o TrateRequest

**Arquivo:** `src/SocketServer.cpp` → `handleClientData()`

Hoje a ponte de integração instancia `TrateRequest` sem contexto de config:

```cpp
// ATUAL:
ParserRequest parsed_req(raw_request);
TrateRequest handler(parsed_req);
```

Com o config do Wesley, o `TrateRequest` precisa saber:
- Qual `client_max_body_size` aplicar (hoje está 1MB hardcoded no `ifPost`)
- Quais métodos são permitidos na rota requisitada
- Quais páginas de erro usar (hoje estão hardcoded como `"www/error/404.html"`)
- Qual o diretório root do servidor

**Opções de implementação (discutir com a companheira):**
- Criar uma struct `ServerConfig` como argumento extra do construtor `TrateRequest`
- Ou passar só os campos necessários (max_body, error_pages, locations)

---

## Prioridade 3 — CGI Não-Bloqueante no SocketServer

**Arquitetura atual (bloqueante — risco de nota zero na avaliação):**

```
poll() detecta POLLIN no client FD
    → handleClientData() chama TrateRequest
        → TrateRequest chama executeCGIGet()
            → fork() + waitpid(pid, 0) ← BLOQUEIA O LOOP INTEIRO
            → read(pipe, ...) em loop    ← BLOQUEIA O LOOP INTEIRO
```

Durante esse bloqueio, nenhum outro cliente é atendido. Com Siege isso é eliminatório.

**A solução é registrar os pipes do CGI no `_poll_fds`:**

```
fork() → filho executa o script
pai:
    → registrar pipefd[0] (stdout do CGI) em _poll_fds com POLLIN
    → guardar associação: pipe_fd → client_fd (para saber a quem responder)
    → retornar ao loop (não bloqueia)

Mais tarde, poll() acorda com POLLIN no pipe_fd:
    → ler o output do CGI
    → waitpid(pid, WNOHANG) para reap sem bloquear
    → montar _client_responses[client_fd]
    → mudar _poll_fds[client_fd] para POLLOUT
```

**O que você precisa adicionar no SocketServer:**

```cpp
// Novo map para rastrear pipes CGI ativos:
std::map<int, int> _cgi_pipe_to_client; // pipe_fd → client_fd
std::map<int, pid_t> _cgi_pipe_to_pid;  // pipe_fd → pid do filho
std::map<int, std::string> _cgi_buffers; // pipe_fd → output acumulado

// Novo método:
void handleCGIRead(size_t index); // chamado quando pipe_fd tem POLLIN
```

**Nota:** Isso exige refatorar `executeCGIGet/Post` para retornar o `pipefd[0]` e o `pid` em vez de bloquear. A companheira precisa saber dessa mudança de interface.

---

## Prioridade 4 — Validação da Avaliação (Scale)

Itens da régua que você é responsável direto:

| Item da Scale | Status | Ação |
|---|---|---|
| `poll()` monitora leitura E escrita ao mesmo tempo | ✅ | — |
| Apenas 1 `poll()` no loop principal | ✅ | — |
| 1 `recv`/`send` por cliente por iteração do `poll` | ✅ | — |
| `errno` NÃO verificado após `recv`/`send` | ✅ | — |
| `recv` e `send` retornam -1 E 0 verificados separadamente | ✅ (corrigido) | — |
| Múltiplas portas via config | ⚠️ | Aguarda Wesley |
| Múltiplos servidores com hostnames diferentes | ⚠️ | Aguarda Wesley |
| Limite de body via config | ⚠️ | Aguarda Wesley |
| Siege -b acima de 99.5% | ⚠️ | Depende do CGI não-bloqueante |
| Sem conexões penduradas | ✅ (timeout 30s) | Testar com Siege |

---

## Prioridade 5 — Testar com Siege Antes da Entrega

```bash
# Instalar
brew install siege

# Teste básico de disponibilidade (deve dar > 99.5%)
siege -b -t 30S http://localhost:8080/

# Verificar se não há memory leak
# Monitorar o processo durante o siege:
top -pid $(pgrep webserv)

# Verificar conexões penduradas:
lsof -i :8080 | grep CLOSE_WAIT
```

Se a disponibilidade cair abaixo de 99.5%, a causa mais provável é o CGI bloqueante (Prioridade 3) ou o `TrateRequest` segfaultando em algum edge case.

---

## Resumo de Dependências

```
Você (Claudio)          Depende de              Para fazer
─────────────────────────────────────────────────────────────
Integrar config         Wesley terminar         Prioridade 1 e 2
CGI não-bloqueante      Companheira refatorar   Prioridade 3
                        executeCGI para retornar
                        pipe_fd + pid
Siege                   Tudo acima              Prioridade 5
```

---

## O que NÃO é sua responsabilidade

- Lógica de negócio dentro do `TrateRequest` (GET/POST/DELETE) → **companheira**
- Parser do arquivo de config → **Wesley**
- Scripts CGI em Python → **companheira**
- Front-end HTML/CSS/JS → **companheira**
