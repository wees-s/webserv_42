# Próximos Passos — Claudio (Socket / Motor de Rede)
### Estado: 2026-05-14

---

## O que já está pronto

- `poll()` não-bloqueante, múltiplas portas, keep-alive, timeouts
- CGI não-bloqueante integrado (`_cgi_pipe_to_client`, `handleCGIRead`)
- `ParserConf` injetado em `SocketServer` e `TrateRequest`
- Loop corrigido (`if POLLIN` + `if POLLOUT` sem `else if`)
- Validação de método por rota via config
- Redirecionamentos 301/302 via config

---

## Bugs ativos — risco de nota zero na avaliação

### BUG 1 — `sendPage` ainda usa `Connection: close`
✅ Já foi resolvido nas ultimas modificações da Beatriz
**Arquivo:** `src/TrateRequest/TrateRequest.cpp` linha final do `sendPage`

```cpp
// ATUAL (quebrado):
header += "Connection: close\r\n\r\n";
```

O SocketServer usa keep-alive — volta para `POLLIN` após enviar. O browser vê
`Connection: close` e fecha a conexão. O servidor fica esperando em um FD morto.

```cpp
// CORREÇÃO:
header += "Connection: keep-alive\r\n\r\n";
```

---

### BUG 2 — `sendPage` com `\r\n` duplicado nas chamadas de `ifGet`
✅ Já foi resolvido nas ultimas modificações da Beatriz
**Arquivo:** `src/TrateRequest/ifGet.cpp`

```cpp
// ATUAL (quebrado) — linhas do /api/curriculum e /api/pid:
sendPage(filename, parser_request.version + " 200 OK\r\n");
//                                                      ^^^^
```

`sendPage` já faz `header = status_header + "\r\n"` internamente. Com o `\r\n`
extra no argumento, a resposta fica:

```
HTTP/1.1 200 OK\r\n
\r\n                  ← linha em branco prematura = fim dos headers
Content-Type: ...     ← tratado como body, não como header
```

O browser falha no `JSON.parse()` e o currículo não carrega.

```cpp
// CORREÇÃO — remover o \r\n do final em todas as chamadas:
sendPage(filename, parser_request.version + " 200 OK");
```

Verificar todas as chamadas de `sendPage` em `ifGet.cpp`, `ifPost.cpp`,
`ifDelete.cpp` e `TrateRequest.cpp` — nenhuma deve passar `\r\n` no
`status_header`.

---

### BUG 3 — `isMethodAllowed` falha para paths que não estão no config
✅ Já foi resolvido nas ultimas modificações da Beatriz
**Arquivo:** `src/TrateRequest/TrateRequest.cpp`

```cpp
if (!config.isMethodAllowed(parser_request.path, parser_request.method))
```

`isMethodAllowed` faz lookup exato por path. Para `/api/curriculum`, o config
tem apenas `/`, `/cgi-bin/`, `/upload`, `/old-path`, `/another-old`. Nenhum
desses é `/api/curriculum`. O fallback vai para `/`, que tem GET/POST/DELETE —
isso funciona. Mas para qualquer path fora do config, `getMethods` retorna
vetor vazio, e `isMethodAllowed` retorna `false` para QUALQUER método,
incluindo GET estático em `/curriculo.css`. Resultado: 405 em todos os assets.

```cpp
// CORREÇÃO em ParserConf::isMethodAllowed — se path não tem location,
// fazer lookup por prefixo antes do fallback para "/":
bool ParserConf::isMethodAllowed(const std::string& path, const std::string& method) const
{
    // Busca exata
    std::map<std::string, LocationConfig>::const_iterator it = _servers[0].locations.find(path);
    if (it != _servers[0].locations.end())
    {
        // encontrou location exata
        for (size_t i = 0; i < it->second.methods.size(); i++)
            if (it->second.methods[i] == method) return true;
        return false;
    }
    
    // Busca por prefixo (ex: /cgi-bin/script.py → /cgi-bin/)
    for (it = _servers[0].locations.begin(); it != _servers[0].locations.end(); ++it)
    {
        if (path.find(it->first) == 0 && it->first != "/")
        {
            for (size_t i = 0; i < it->second.methods.size(); i++)
                if (it->second.methods[i] == method) return true;
            return false;
        }
    }
    
    // Fallback para "/"
    it = _servers[0].locations.find("/");
    if (it != _servers[0].locations.end())
    {
        for (size_t i = 0; i < it->second.methods.size(); i++)
            if (it->second.methods[i] == method) return true;
    }
    return false;
}
```

---

### BUG 4 — `handleCGIRead`: pipe CGI não está em `_client_last_activity`
✅ Já foi resolvido nas ultimas modificações da Beatriz
**Arquivo:** `src/SocketServer.cpp` → `checkTimeouts()`

`checkTimeouts` itera todos os FDs que não são server sockets. Pipes de CGI
são adicionados ao `_poll_fds` mas NÃO têm entrada em `_client_last_activity`.
A condição `_client_last_activity.count(fd)` protege o crash, mas se um pipe
de CGI não fechar em 30s, o timeout vai `closeConnection(i)` nele — o que
apaga o FD do vetor sem fazer o `waitpid` do filho. Processo zumbi garantido.

```cpp
// CORREÇÃO em checkTimeouts — ignorar pipes de CGI:
if (_cgi_pipe_to_client.count(fd))
    continue; // pipe de CGI — não aplicar timeout de cliente aqui
if (_client_last_activity.count(fd) && ...)
    closeConnection(i);
```

---

### BUG 5 — `_client_buffers` não é apagado no path do CGI
✅ Já foi resolvido nas ultimas modificações do Claudio
**Arquivo:** `src/SocketServer.cpp` → `handleClientData()`

No branch `if (handler.hasCGI())`, o request é processado mas
`_client_buffers[fd].erase(0, expected_total_size)` não é chamado. Se o
cliente fizer outra requisição em keep-alive, o buffer antigo ainda estará lá,
e o próximo `_client_buffers[fd].find("\r\n\r\n")` vai achar o `\r\n\r\n` do
request anterior.

```cpp
// CORREÇÃO — adicionar o erase também no branch CGI:
if (handler.hasCGI())
{
    ...
    _cgi_buffers[pipe_fd] = "";
    _client_buffers[fd].erase(0, expected_total_size); // ← adicionar aqui
}
```

---

## O que falta implementar para a avaliação

### 1. Leitura do `.conf` via `argv[1]`
**Arquivo:** `src/main.cpp`

O argumento está comentado. A avaliação pede `./webserv conf/default.conf`.

```cpp
// ATUAL (comentado):
(void)argc;
(void)argv;
ParserConf conf;

// FUTURO:
if (argc != 2) { std::cerr << "Usage: ./webserv <config>\n"; return 1; }
ParserConf conf(argv[1]);
```

Isso depende do Wesley implementar o parser real do `.conf`. Enquanto isso,
o `ParserConf()` default hardcoded funciona para testar.

---

### 2. Múltiplas portas via config
O `default.conf` tem dois blocos `server {}` ambos na porta 8080 — conflito
que não está sendo tratado. A avaliação vai testar múltiplas portas **diferentes**.

No `default.conf`, trocar o segundo bloco para porta diferente:
```
server { listen 8081; ... }
```

E garantir que `ParserConf::getPorts()` retorna todas as portas de todos os
blocos server (hoje só retorna as do primeiro bloco).

---

### 3. Siege antes da entrega

```bash
# Com servidor rodando em background:
./webserv &

# Teste de disponibilidade (meta: > 99.5%)
siege -b -t 30S http://localhost:8080/

# Monitorar memória durante siege:
top -pid $(pgrep webserv)

# Verificar conexões penduradas:
lsof -i :8080 | grep CLOSE_WAIT
```

---

## Checklist da avaliação vs estado atual

| Item da Scale | Status |
|---|---|
| `poll()` monitora leitura E escrita simultaneamente | ✅ |
| Apenas 1 `poll()` no loop | ✅ |
| 1 `recv`/`send` por cliente por iteração | ✅ |
| `errno` NÃO verificado após `recv`/`send` | ✅ |
| `recv`/`send` verificam -1 e 0 separadamente | ✅ |
| Servidor não trava em conexão inválida | ✅ (timeout 30s) |
| GET, POST, DELETE funcionam | ✅ (BUG 1, 2 resolvidos) |
| CGI GET e POST funcionam | ⚠️ (não-bloqueante integrado, testar) |
| Múltiplas portas via config | ⚠️ (aguarda Wesley + BUG dos 2 blocos) |
| Limite de body via config | ✅ (lê de `_clientMaxBodySize`) |
| Páginas de erro customizadas | ✅ (lê de `errorPages`) |
| Siege > 99.5% | ⚠️ (testar) |
| Sem memory leak | ⚠️ (verificar com `leaks` após siege) |
| Sem conexões penduradas | ✅ (BUG 4 resolvido pela Beatriz com 504 Timeout) |

---

## Ordem de execução

```
1. Testar: curl + browser + testes Python
2. Testar: siege -b -t 30S
3. Preparar para a avaliação
```