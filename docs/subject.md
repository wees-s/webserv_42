# Regras gerais

- Seu programa não deve travar sob nenhuma circunstância (mesmo que fique sem memória) ou terminar inesperadamente. Se isso ocorrer, seu projeto será considerado não funcional e sua nota será 0.
- Você deve enviar um Makefile que compile seus arquivos de origem. Ele não deve realizar uma religação desnecessária.
- Seu Makefile deve conter pelo menos as regras: `$(NAME)`, `all`, `clean`, `fclean` e `re`.
- Compile seu código com `c++` e as flags `-Wall -Wextra -Werror`
- Seu código deve estar em conformidade com o padrão **C++ 98** e ainda deve compilar ao adicionar a flag `-std=c++98`.
- Certifique-se de aproveitar o máximo possível dos recursos do C++ (por exemplo, escolha `<cstring>` em vez de `<string.h>`). Você está autorizado a usar funções C, mas sempre prefira suas versões C++ se possível.
- Qualquer biblioteca externa e bibliotecas Boost são proibidas.

# Parte obrigatória

**Nome do programa:** webserv  
**Arquivos para entregar:** Makefile, *.{h, hpp}, *.cpp, *.tpp, *.ipp, arquivos de configuração  
**Makefile:** NAME, all, clean, fclean, re  
**Argumentos:** [Um arquivo de configuração]

## Funções externas autorizadas

Toda a funcionalidade deve ser implementada em C++ 98.

```
execve, pipe, strerror, gai_strerror, errno, dup,
dup2, fork, socketpair, htons, htonl, ntohs, ntohl,
select, poll, epoll (epoll_create, epoll_ctl,
epoll_wait), kqueue (kqueue, kevent), socket,
accept, listen, send, recv, chdir, bind, connect,
getaddrinfo, freeaddrinfo, setsockopt, getsockname,
getprotobyname, fcntl, close, read, write, waitpid,
kill, signal, access, stat, open, opendir, readdir
e closedir.
```

**Libft autorizada:** n/a  
**Descrição:** Um servidor HTTP em C++ 98

Você deve escrever um servidor HTTP em C++ 98. Seu executável deve ser executado da seguinte forma:

```bash
./webserv [arquivo de configuração]
```

Mesmo que `poll()` seja mencionado no subject e na régua de avaliação, você pode usar qualquer função equivalente, como `select()`, `kqueue()` ou `epoll()`.

Por favor, leia os RFCs que definem o protocolo HTTP e execute testes com telnet e NGINX antes de começar este projeto. Embora você não seja obrigado a implementar todos os RFCs, lê-los ajudará você a desenvolver os recursos necessários. O HTTP 1.0 é sugerido como um ponto de referência, mas não é obrigatório.

## Requisitos

- Seu programa deve usar um arquivo de configuração, fornecido como um argumento na linha de comando, ou disponível em um caminho padrão.
- Você não pode `execve` outro servidor web.
- Seu servidor deve permanecer não bloqueante em todos os momentos e lidar adequadamente com desconexões de clientes quando necessário.
- Ele deve ser não bloqueante e usar apenas 1 `poll()` (ou equivalente) para todas as operações de I/O entre os clientes e o servidor (incluindo listen).
- `poll()` (ou equivalente) deve monitorar a leitura e a escrita simultaneamente.
- Você nunca deve fazer uma operação de leitura ou escrita sem passar por `poll()` (ou equivalente).
- Verificar o valor de errno para ajustar o comportamento do servidor é estritamente proibido após realizar uma operação de leitura ou escrita.
- Você não é obrigado a usar `poll()` (ou equivalente) antes de `read()` para recuperar seu arquivo de configuração.

Como você tem que usar descritores de arquivo não bloqueantes, é possível usar funções `read/recv` ou `write/send` sem `poll()` (ou equivalente), e seu servidor não ficaria bloqueando. Mas isso consumiria mais recursos do sistema. Assim, se você tentar `read/recv` ou `write/send` em qualquer descritor de arquivo sem usar `poll()` (ou equivalente), sua nota será 0.

- Ao usar `poll()` ou qualquer chamada equivalente, você pode usar todas as macros ou funções auxiliares associadas (por exemplo, `FD_SET` para `select()`).
- Uma solicitação ao seu servidor nunca deve travar indefinidamente.
- Seu servidor deve ser compatível com navegadores web padrão de sua escolha.
- NGINX pode ser usado para comparar cabeçalhos e comportamentos de resposta (preste atenção às diferenças entre as versões HTTP).
- Seus códigos de status de resposta HTTP devem ser precisos.
- Seu servidor deve ter páginas de erro padrão se nenhuma for fornecida.
- Você não pode usar `fork` para nada além de CGI (como PHP ou Python, e assim por diante).

- Você deve ser capaz de servir um site totalmente estático.
- Os clientes devem ser capazes de enviar arquivos.
- Você precisa de pelo menos os métodos GET, POST e DELETE.
- Teste seu servidor para garantir que ele permaneça disponível em todos os momentos.
- Seu servidor deve ser capaz de ouvir várias portas para fornecer conteúdo diferente (ver Arquivo de configuração).

Escolhemos deliberadamente oferecer apenas um subconjunto do HTTP RFC. Neste contexto, o recurso de host virtual é considerado fora do escopo. Mas você pode implementá-lo se quiser.

### Apenas para MacOS

Como o macOS lida com `write()` de forma diferente de outros sistemas operacionais baseados em Unix, você está autorizado a usar `fcntl()`. Você deve usar descritores de arquivo no modo não bloqueante para obter um comportamento semelhante ao de outros sistemas operacionais Unix.

No entanto, você só pode usar `fcntl()` com as seguintes flags:
- `F_SETFL`
- `O_NONBLOCK`
- `FD_CLOEXEC`

Qualquer outra flag é proibida.

## Arquivo de configuração

Você pode se inspirar na seção 'server' do arquivo de configuração do NGINX.

No arquivo de configuração, você deve ser capaz de:

- Definir todos os pares interface:porta nos quais seu servidor irá ouvir (definindo vários sites servidos pelo seu programa).
- Configurar páginas de erro padrão.
- Definir o tamanho máximo permitido para os corpos de solicitação do cliente.
- Especificar regras ou configurações em um URL/rota (nenhum regex é necessário aqui), para um site, entre os seguintes:
  - Lista de métodos HTTP aceitos para a rota.
  - Redirecionamento HTTP.
  - Diretório onde o arquivo solicitado deve estar localizado (por exemplo, se o URL `/kapouet` estiver enraizado em `/tmp/www`, o URL `/kapouet/pouic/toto/pouet` procurará por `/tmp/www/pouic/toto/pouet`).
  - Ativar ou desativar a listagem de diretórios.
  - Arquivo padrão a ser servido quando o recurso solicitado é um diretório.
  - O envio de arquivos dos clientes para o servidor é autorizado e o local de armazenamento é fornecido.
  - Execução de CGI, com base na extensão do arquivo (por exemplo, .php). Aqui estão algumas observações específicas sobre CGIs:
    - Você está se perguntando o que é um CGI?
    - Observe atentamente as variáveis de ambiente envolvidas na comunicação servidor web-CGI. A solicitação completa e os argumentos fornecidos pelo cliente devem estar disponíveis para o CGI.
    - Apenas lembre-se de que, para solicitações em partes, seu servidor precisa desagrupá-las, o CGI esperará EOF como o final do corpo.
    - O mesmo se aplica à saída do CGI. Se nenhum `content_length` for retornado do CGI, EOF marcará o final dos dados retornados.
    - O CGI deve ser executado no diretório correto para acesso a arquivos de caminho relativo.
    - Seu servidor deve suportar pelo menos um CGI (php-CGI, Python e assim por diante).

Você deve fornecer arquivos de configuração e arquivos padrão para testar e demonstrar que todos os recursos funcionam durante a avaliação. Você pode ter outras regras ou informações de configuração em seu arquivo (por exemplo, um nome de servidor para um site se você planeja implementar hosts virtuais).

Se você tiver uma pergunta sobre um comportamento específico, pode comparar o comportamento do seu programa com o do NGINX. Fornecemos um pequeno testador. Usá-lo não é obrigatório se tudo funcionar bem com seu navegador e testes, mas pode ajudá-lo a encontrar e corrigir bugs.

A resiliência é fundamental. Seu servidor deve permanecer operacional em todos os momentos.

Não teste com apenas um programa. Escreva seus testes em uma linguagem mais adequada, como Python ou Golang, entre outras, mesmo em C ou C++ se você preferir.

# Parte bônus

Aqui estão alguns recursos adicionais que você pode implementar:

- Suporte a cookies e gerenciamento de sessão (forneça exemplos simples).
- Lidar com vários tipos de CGI.

A parte bônus só será avaliada se a parte obrigatória estiver totalmente concluída sem problemas. Se você não atender a todos os requisitos obrigatórios, sua parte bônus não será avaliada.
