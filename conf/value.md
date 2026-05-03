***O QUE ENTREGAR:***

ServerConfig:   

  **port**: 8080   
  ___
  **server_name**: localhost   
  ___
  **root**: www/   
  ___
  **locations**:   
    - path: /   
      methods: [GET, POST, DELETE]   
  ___
  **error_pages**:   
    404 -> www/404.html   
  ___