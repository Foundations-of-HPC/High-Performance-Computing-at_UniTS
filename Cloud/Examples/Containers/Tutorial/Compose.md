# Docker Compose Tutorial

## Introduction
Docker Compose is a tool for defining and running multi-container Docker applications. With a simple YAML configuration file, you can easily manage and orchestrate multiple services, networks, and volumes.

## 1. **Installing Docker Compose**
Docker Compose is included with Docker Desktop. To install it separately on Linux:
```bash
sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose
```
Verify the installation:
```bash
docker-compose --version
```

## 2. **Basic Usage of Docker Compose**
A `docker-compose.yml` file defines the services, networks, and volumes for an application. Use the following steps:

### **2.1 Basic Example: Running a Simple Web App**
Create a `docker-compose.yml` file:
```yaml
version: '3'
services:
  web:
    image: nginx
    ports:
      - "8080:80"
```
Start the service:
```bash
docker-compose up -d
```
Check running containers:
```bash
docker-compose ps
```
Stop and remove services:
```bash
docker-compose down
```

## 3. **Intermediate Usage: Multi-Service Application**
A common use case is running a web application with a database.

### **3.1 Example: Web App with Database**
Create a `docker-compose.yml` file:
```yaml
version: '3.8'
services:
  web:
    image: python:3.9
    working_dir: /app
    volumes:
      - .:/app
    ports:
      - "5000:5000"
    depends_on:
      - db
    command: python3 app.py
  db:
    image: postgres
    restart: always
    environment:
      POSTGRES_USER: user
      POSTGRES_PASSWORD: password
      POSTGRES_DB: mydatabase
```
Start the application:
```bash
docker-compose up -d
```
Check logs:
```bash
docker-compose logs -f
```

## 4. **Advanced Usage: Complex Example with Networks, Volumes, and Resource Limits**
In a more complex scenario, services are isolated into networks, and volumes persist data, while also applying CPU and memory constraints.

### **4.1 Example: Multi-Tier Application with Resource Limits**
```yaml
version: '3.8'
services:
  frontend:
    image: nginx
    depends_on:
      - backend
    networks:
      - app_network
    ports:
      - "80:80"
    deploy:
      resources:
        limits:
          cpus: "0.5"
          memory: "512M"
  backend:
    image: node:16
    working_dir: /app
    volumes:
      - backend_data:/app
    networks:
      - app_network
    depends_on:
      - db
    deploy:
      resources:
        limits:
          cpus: "1.0"
          memory: "1G"
  db:
    image: mysql:5.7
    restart: always
    environment:
      MYSQL_ROOT_PASSWORD: rootpassword
      MYSQL_DATABASE: mydb
    volumes:
      - db_data:/var/lib/mysql
    networks:
      - app_network
    deploy:
      resources:
        limits:
          cpus: "1.0"
          memory: "2G"
volumes:
  backend_data:
  db_data:
networks:
  app_network:
```
Bring up the stack:
```bash
docker-compose up -d
```
Inspect the network:
```bash
docker network ls
```
Remove everything:
```bash
docker-compose down -v
```

## 5. **Using Docker Compose for Workflows**
Docker Compose can be used to define workflows with task dependencies and automation.

### **5.1 Example: Data Processing Pipeline**
```yaml
version: '3.8'
services:
  data_fetch:
    image: curlimages/curl
    command: curl -o /data/input.txt https://example.com/data.txt
    volumes:
      - data_volume:/data
  processor:
    image: python:3.9
    depends_on:
      - data_fetch
    volumes:
      - data_volume:/data
    command: python3 /data/process.py
  storage:
    image: postgres
    environment:
      POSTGRES_USER: user
      POSTGRES_PASSWORD: password
      POSTGRES_DB: results
    depends_on:
      - processor
volumes:
  data_volume:
```
This workflow fetches data, processes it, and stores the result in a database. Start it with:
```bash
docker-compose up
```

## 6. **Resource Constraints: CPU Binding and Memory Limits**
Docker Compose allows defining CPU and memory constraints for better resource management.

### **6.1 Setting CPU and Memory Limits**
```yaml
version: '3.8'
services:
  app:
    image: myapp
    deploy:
      resources:
        limits:
          cpus: "1.5"
          memory: "1G"
```
This restricts the container to 1.5 CPU cores and 1GB of RAM.

### **6.2 Binding a Container to Specific CPUs**
```yaml
version: '3.8'
services:
  worker:
    image: myworker
    cpuset: "0,2"
```
This ensures the container only runs on CPU cores 0 and 2.

### **6.3 Memory Swapping**
```yaml
version: '3.8'
services:
  db:
    image: mysql:5.7
    deploy:
      resources:
        limits:
          memory: "512M"
        reservations:
          memory: "256M"
```
This limits the database container to 512MB RAM while reserving 256MB at minimum.

## Conclusion

For further learning, check the [official Docker Compose documentation](https://docs.docker.com/compose/).
