**TESTE DE PORTAS E SERVERS**
curl -i http://localhost:8080/templates.html
curl -i http://localhost:8081/templates.html
curl -i http://test:8082/templates.html
curl -i http://test:8083/templates.html

testar server com name diferente:
    1. etc/hosts -> 127.0.0.1 test
    2. curl -H "Host: test" http://localhost:8081/


**TESTE DE MÉTODOS PERMITIDOS**
curl -i -X GET http://localhost:8080/api/curriculum
curl -i -X POST http://localhost:8080/api/curriculum
curl -i -X DELETE http://localhost:8080/api/curriculum


**TESTE DE REDIRECIONAMENTO**
curl -i -L http://localhost:8080/old-path
curl -i -L http://localhost:8080/another-old


**TESTE GET**

*Arquivo normal:*
curl -i http://localhost:8080/

*CGI:*
curl -i -X GET http://localhost:8080/cgi-bin/cgiGet.py

*List diretório:*
curl -i http://localhost:8080/error


**TESTE POST**

*JSON e arquivo*
--Melhor pelo navegador

*CGI POST*
curl -i -X POST http://localhost:8080/cgi-bin/cgiPost.py


**TESTE DELETE**
curl -i -X DELETE http://localhost:8080/api/curriculum


**TESTE CGI**

*Erro CGI vazio*
curl -i -X GET http://localhost:8080/cgi-bin/tests/test_empty_output.py

*Erro CGI com saída de erro*
curl -i -X GET http://localhost:8080/cgi-bin/tests/test_exit_error.py

*Erro CGI com loop infinito*
curl -i -X GET http://localhost:8080/cgi-bin/tests/test_infinite_loop.py

*Erro CGI com erro de syntax*
curl -i -X GET http://localhost:8080/cgi-bin/tests/test_syntax_error.py

*CGI multipart*
curl -i -X POST -F "bia=123" -F "wes=456" -F "claudio=789" -F "arquivo=@www/cgi-bin/tests/test_file.txt" http://localhost:8080/cgi-bin/tests/test_multipart.py

