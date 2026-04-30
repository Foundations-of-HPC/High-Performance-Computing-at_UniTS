FROM python:3.11-slim

LABEL maintainer="HPCC 2025/2026 course"
LABEL description="Stateless compute service for linear-system solves"
LABEL version="1.0"

WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir --compile -r requirements.txt

COPY server.py .

RUN useradd --create-home --shell /bin/bash appuser
USER appuser

EXPOSE 8000

CMD ["uvicorn", "server:app", "--host", "0.0.0.0", "--port", "8000"]
