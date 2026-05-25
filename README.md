*Este projeto foi criado como parte do currículo da 42 por <bedantas, clados-s, wedos-sa>.*

# webserv_42

## Descrição

O webserv_42 é um servidor HTTP implementado em C++98 que utiliza multiplexação de I/O (select()) para gerenciar múltiplas conexões simultâneas. O projeto visa replicar funcionalidades essenciais de servidores web como nginx, incluindo:

- **Servidor virtual:** Suporte a múltiplos servidores com diferentes portas e hostnames
- **Rotas e locations:** Configuração de rotas com métodos HTTP permitidos por location
- **CGI:** Execução de scripts CGI (Python, PHP) com suporte a GET e POST
- **Upload de arquivos:** Suporte a multipart/form-data para upload
- **Páginas de erro:** Configuração de páginas de erro personalizadas (400, 403, 404, 405, 413, 500, 504)
- **Redirecionamentos:** Suporte a redirecionamentos 301 (permanente) e 302 (temporário)
- **Limites:** Configuração de client_max_body_size para limitar tamanho do corpo da requisição

O servidor lê um arquivo de configuração (`default.conf`) e serve arquivos estáticos do diretório `www/`, além de fornecer uma API REST para gerenciamento de currículos.

## Instruções

### Pré-requisitos

- Linux com `make` e um compilador C++ (`c++`)
- Executar os comandos **na raiz do repositório** (`webserv_42/`), para os caminhos `www/` funcionarem

### Compilação

```bash
make
```

O binário gerado é `./webserv`.

Para limpar objetos e o executável:

```bash
make fclean
```

(`fclean` também remove `www/data/curriculum.json` e arquivos em `www/uploads/`, conforme o `Makefile`.)

### Execução

```bash
./webserv default.conf
```

O servidor escuta nas portas configuradas no arquivo `default.conf` (por padrão: 8080, 8081, 8082, 8083). Encerre com **Ctrl+C** (SIGINT).

### Testar no navegador

**Páginas estáticas**

- `http://localhost:8080/` — página inicial (`www/index.html`)
- `http://localhost:8080/templates.html` — página de templates de currículo
- `http://localhost:8080/classico.html` — template clássico de currículo
- Qualquer arquivo sob `www/`

**API do currículo**

- Preencha o formulário em `http://localhost:8080/classico.html` e clique em "Salvar" para fazer POST
- Clique em "Limpar dados" para fazer DELETE
- Os dados são salvos em `www/data/curriculum.json` e arquivos em `www/uploads/`

**Redirecionamentos**

- `http://localhost:8080/old-path` — redireciona 301 para `/index.html`
- `http://localhost:8080/another-old` — redireciona 302 para `/templates.html`

**CGI**

- `http://localhost:8080/cgi-bin/cgiGet.py` — executa script CGI GET
- Formulários POST em páginas que usam CGI executam scripts POST

**Erros**

- `http://localhost:8080/arquivo-inexistente` — página 404
- Tente acessar `/api/curriculum` com método não permitido para ver 405

### Testar com curl (opcional)

Para testes específicos que não são fáceis no navegador:

**Método não permitido (405)**

```bash
curl -i -X PUT http://localhost:8080/
```

### Resolução de problemas

Se ao subir o servidor aparecer erro de bind na porta, outro processo já está usando a porta. Pare o processo:

```bash
pkill -f webserv
```

Ou libere a porta antes de rodar `./webserv` novamente.

## Recursos

### Referências

- [HTTP Status Codes - MDN](https://developer.mozilla.org/en-US/docs/Web/HTTP/Status)
- [CGI Specification](https://www.w3.org/CGI/)
- [select() - Linux man page](https://man7.org/linux/man-pages/man2/select.2.html)
- [HTTP/1.1 - RFC 2616](https://www.rfc-editor.org/rfc/rfc2616)

### Uso de Inteligência Artificial

A IA (Cascade) foi utilizada para:

- **Debugging de configuração:** Diagnóstico de erros de parsing no arquivo `default.conf` (comentários sendo tokenizados, CRLF vs LF)
- **Explicação de conceitos:** Esclarecimento sobre virtual hosting, server_name, Host header e resolução DNS
- **Correção de código:** Modificação do tokenizer em `TokenConf.cpp` para ignorar comentários corretamente
- **Troubleshooting:** Identificação de processos bloqueando portas e configuração do arquivo hosts do Windows
- **Refatoração:** Limpeza de comentários desnecessários no código fonte para melhor legibilidade
- **Documentação:** Elaboração de exemplos de teste com curl para validação das funcionalidades

A IA não foi utilizada para implementar funcionalidades principais do servidor, apenas para suporte em debugging e compreensão de conceitos.
