# webserv_42

Servidor HTTP em C++98 (projeto 42). Este README só cobre **como compilar e testar** o que já existe no código.

## Pré-requisitos

- Linux com `make` e um compilador C++ (`c++`).
- Executar os comandos **na raiz do repositório** (`webserv_42/`), para os caminhos `www/` funcionarem.

## Compilar

```bash
make
```

O binário gerado é `./webserv`.

Para limpar objetos e o executável:

```bash
make fclean
```

(`fclean` também remove `www/data/curriculum.json` e arquivos em `www/uploads/`, conforme o `Makefile`.)

## Subir o servidor

```bash
./webserv
```

- Escuta em **`http://127.0.0.1:8080`** (porta fixa no código).
- Encerre com **Ctrl+C** (SIGINT).

## Testar no navegador

Abra:

- `http://127.0.0.1:8080/` — página inicial (`www/index.html`).
- Qualquer arquivo sob `www/`, por exemplo `http://127.0.0.1:8080/classico.html`.

Se o arquivo não existir, o servidor responde com a página de erro configurada (`www/error/404.html`).

## Testar com `curl`

**GET da raiz**

```bash
curl -i http://127.0.0.1:8080/
```

**API do currículo (JSON)**

O handler `GET /api/curriculum` devolve `www/data/curriculum.json` se existir; caso contrário usa `www/data/default_curriculum.json`.

```bash
curl -s http://127.0.0.1:8080/api/curriculum | head
```

**POST `/api/curriculum`** — exemplo `application/x-www-form-urlencoded` (corpo vira JSON salvo em `www/data/curriculum.json`):

```bash
curl -i -X POST http://127.0.0.1:8080/api/curriculum \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data "nome=Teste&titulo=Dev"
```

**POST com upload (multipart)** — envia um arquivo para `www/uploads/` e atualiza o JSON (campo `photo` vira URL em `photoUrl`):

```bash
curl -i -X POST http://127.0.0.1:8080/api/curriculum \
  -F "photo=@/caminho/para/uma/imagem.png"
```

**DELETE `/api/curriculum`** — remove `curriculum.json` (se existir) e limpa `www/uploads/`:

```bash
curl -i -X DELETE http://127.0.0.1:8080/api/curriculum
```

**Método não suportado** (ex.: `PUT`) — deve cair na resposta 405 usando `www/error/405.html`.

```bash
curl -i -X PUT http://127.0.0.1:8080/
```

---

Se ao subir o servidor aparecer erro de bind na porta 8080, outro processo já está usando a porta; pare esse processo ou libere a porta antes de rodar `./webserv` de novo.
