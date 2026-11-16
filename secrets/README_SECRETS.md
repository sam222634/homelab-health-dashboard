# Gestión de Secretos con Docker Secrets (`/run/secrets/`)

Esta carpeta almacena localmente los archivos de credenciales que Docker monta en memoria RAM (`tmpfs`) dentro de los contenedores bajo `/run/secrets/`.

> **IMPORTANTE:** Los archivos `.txt` dentro de esta carpeta **NUNCA** deben subirse a Git (están excluidos mediante `.gitignore`).

## Cómo generar los secretos localmente antes de levantar el stack

Ejecuta desde la raíz del proyecto:

```bash
# 1. Crear archivo con la contraseña del usuario administrador de InfluxDB (mínimo 8 caracteres)
echo "TuPasswordSeguroDeInfluxDB" > secrets/influx_admin_password.txt

# 2. Generar un token aleatorio de 64 caracteres hexadecimales para inicializar InfluxDB
openssl rand -hex 32 > secrets/influx_admin_token.txt

# 3. Para la primera ejecución puedes copiar el mismo token o luego reemplazarlo por un token de solo escritura (WRITE-ONLY) creado en la interfaz de InfluxDB (puerto 8086):
cp secrets/influx_admin_token.txt secrets/influx_write_token.txt

# 4. Restringir permisos de lectura únicamente a tu usuario en Linux
chmod 600 secrets/*.txt
```

## Cómo levantar el stack completo

```bash
cp .env.example .env
docker compose -f docker-compose.stack.yml up -d --build
```
