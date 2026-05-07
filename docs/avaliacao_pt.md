# Escala para o projeto webserv

Você deve avaliar 3 estudantes nesta equipe.

## Introdução

Por favor, cumpra as seguintes regras:

- Mantenha-se educado, cortês, respeitoso e construtivo durante todo o processo de avaliação. O bem-estar da comunidade depende disso.
- Identifique com o estudante ou grupo cujo trabalho está sendo avaliado as possíveis disfunções em seu projeto. Dedique tempo para discutir e debater os problemas que possam ter sido identificados.
- Você deve considerar que pode haver diferenças em como seus pares podem ter entendido as instruções do projeto e o escopo de suas funcionalidades. Mantenha sempre uma mente aberta e avalie-os da forma mais honesta possível. A pedagogia é útil apenas e somente se a avaliação entre pares for feita seriamente.

## Diretrizes

- Avalie apenas o trabalho que foi entregue no repositório Git do estudante ou grupo avaliado.
- Verifique duas vezes se o repositório Git pertence ao(s) estudante(s). Certifique-se de que o projeto é o esperado. Além disso, verifique se 'git clone' é usado em uma pasta vazia.
- Verifique cuidadosamente se nenhum alias malicioso foi usado para enganá-lo e fazer você avaliar algo que não é o conteúdo do repositório oficial.
- Para evitar surpresas e se aplicável, revise juntos quaisquer scripts usados para facilitar a avaliação (scripts para teste ou automação).
- Se você não completou a tarefa que vai avaliar, você deve ler todo o subject antes de iniciar o processo de avaliação.
- Use as flags disponíveis para relatar um repositório vazio, um programa não funcional, um erro de Norm, trapaça, e assim por diante. Nestes casos, o processo de avaliação termina e a nota final é 0, ou -42 em caso de trapaça. No entanto, exceto por trapaça, os estudantes são fortemente encorajados a revisar juntos o trabalho que foi entregue, a fim de identificar quaisquer erros que não devem ser repetidos no futuro.
- Lembre-se que durante a defesa, nenhum segfault, nenhuma outra terminação inesperada, prematura, incontrolada ou inesperada do programa, caso contrário a nota final é 0. Use a flag apropriada. Você nunca deve editar nenhum arquivo exceto o arquivo de configuração se ele existir. Se você quiser editar um arquivo, dedique tempo para explicar as razões com o estudante avaliado e certifique-se de que ambos estão de acordo com isso.
- Você também deve verificar a ausência de vazamentos de memória. Qualquer memória alocada no heap deve ser devidamente liberada antes do fim da execução. Você tem permissão para usar qualquer uma das diferentes ferramentas disponíveis no computador, como leaks, valgrind ou e_fence. Em caso de vazamentos de memória, marque a flag apropriada.

## Anexos

- [subject.pdf](https://github.com/rphlr/42-Subjects/)
- [tester](https://github.com/rphlr/42-Subjects/)
- [ubuntu_cgi_tester](https://github.com/rphlr/42-Subjects/)
- [cgi_tester](https://github.com/rphlr/42-Subjects/)
- [ubuntu_tester](https://github.com/rphlr/42-Subjects/)

---

## Parte Obrigatória

### Verifique o código e faça perguntas

- Lance a instalação do siege com homebrew.
- Peça explicações sobre o básico de um servidor HTTP.
- Pergunte qual função o grupo usou para Multiplexação de I/O.
- Peça uma explicação de como funciona select() (ou equivalente).
- Pergunte se eles usam apenas um select() (ou equivalente) e como eles gerenciaram o servidor para aceitar e o cliente para ler/escrever.
- O select() (ou equivalente) deve estar no loop principal e deve verificar descritores de arquivo para leitura e escrita AO MESMO TEMPO. Se não, a nota é 0 e o processo de avaliação termina agora.
- Deve haver apenas uma leitura ou uma escrita por cliente por select() (ou equivalente). Peça ao grupo para mostrar o código do select() (ou equivalente) para a leitura e escrita de um cliente.
- Procure por todos os read/recv/write/send em um socket e verifique que, se um erro for retornado, o cliente é removido.
- Procure por todos os read/recv/write/send e verifique se o valor retornado é corretamente verificado (verificar apenas valores -1 ou 0 não é suficiente, ambos devem ser verificados).
- Se errno é verificado após read/recv/write/send, a nota é 0 e o processo de avaliação termina agora.
- Escrever ou ler QUALQUER descritor de arquivo sem passar pelo select() (ou equivalente) é estritamente PROIBIDO.
- O projeto deve compilar sem nenhum problema de re-link. Se não, use a flag 'Invalid compilation'.
- Se algum ponto não estiver claro ou não estiver correto, a avaliação para.

### Configuração

No arquivo de configuração, verifique se você pode fazer o seguinte e teste o resultado:

- Pesquise a lista de códigos de status de resposta HTTP na internet. Durante esta avaliação, se qualquer código de status estiver errado, não dê pontos relacionados.
- Configure múltiplos servidores com diferentes portas.
- Configure múltiplos servidores com diferentes hostnames (use algo como: curl --resolve example.com:80:127.0.0.1 http://example.com/).
- Configure página de erro padrão (tente mudar o erro 404).
- Limite o corpo do cliente (use: curl -X POST -H "Content-Type: plain/text" --data "BODY IS HERE escreva algo mais curto ou mais longo que o limite do corpo").
- Configure rotas em um servidor para diferentes diretórios.
- Configure um arquivo padrão para procurar se você pedir um diretório.
- Configure uma lista de métodos aceitos para uma certa rota (ex: tente deletar algo com e sem permissão).

### Verificações básicas

Usando telnet, curl, arquivos preparados, demonstre que as seguintes funcionalidades funcionam corretamente:

- Requisições GET, POST e DELETE devem funcionar.
- Requisições DESCONHECIDAS não devem resultar em um crash.
- Para cada teste você deve receber o código de status apropriado.
- Faça upload de algum arquivo para o servidor e receba-o de volta.

### Verifique CGI

Preste atenção ao seguinte:

- O servidor está funcionando bem usando um CGI.
- O CGI deve ser executado no diretório correto para acesso a arquivos de caminho relativo.
- Com a ajuda dos estudantes você deve verificar que tudo está funcionando corretamente. Você tem que testar o CGI com os métodos "GET" e "POST".
- Você precisa testar com arquivos contendo erros para ver se o tratamento de erros funciona corretamente. Você pode usar um script contendo um loop infinito ou um erro; você é livre para fazer quaisquer testes que quiser dentro dos limites de aceitabilidade que permanecem a seu critério. O grupo sendo avaliado deve ajudá-lo com isso.

O servidor nunca deve travar e um erro deve ser visível em caso de problema.

### Verifique com um navegador

- Use o navegador de referência da equipe. Abra a parte de rede dele e tente conectar ao servidor usando-o.
- Olhe para o cabeçalho da requisição e o cabeçalho da resposta.
- Deve ser compatível para servir um site totalmente estático.
- Tente uma URL errada no servidor.
- Tente listar um diretório.
- Tente uma URL redirecionada.
- Tente qualquer coisa que você quiser.

### Problemas de porta

- No arquivo de configuração configure múltiplas portas e use diferentes sites. Use o navegador para garantir que a configuração funciona conforme esperado e mostra o site correto.
- Na configuração, tente configurar a mesma porta múltiplas vezes. Não deve funcionar.
- Lance múltiplos servidores ao mesmo tempo com diferentes configurações mas com portas em comum. Funciona? Se funcionar, pergunte por que o servidor deve funcionar se uma das configurações não está funcional. Continue.

### Siege & teste de estresse

- Use Siege para rodar alguns testes de estresse.
- A disponibilidade deve ser acima de 99.5% para um GET simples em uma página vazia com um siege -b nessa página.
- Verifique se não há vazamento de memória (Monitore o uso de memória do processo. Não deve subir indefinidamente).
- Verifique se não há conexão pendurada.
- Você deve ser capaz de usar siege indefinidamente sem ter que reiniciar o servidor (dê uma olhada em siege -b).

---

## Parte bônus

Avalie a parte bônus se, e somente se, a parte obrigatória tiver sido inteira e perfeitamente feita, e o gerenciamento de erros lida com uso inesperado ou ruim. Caso todos os pontos obrigatórios não tenham sido passados durante a defesa, os pontos bônus devem ser totalmente ignorados.

### Cookies e sessão

Existe um sistema de sessão e cookies funcionando no servidor web.

### CGI

Existe mais de um sistema CGI.
