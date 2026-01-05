FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y g++ make tzdata && rm -rf /var/lib/apt/lists/*

RUN ln -fs /usr/share/zoneinfo/Asia/Novosibirsk /etc/localtime

WORKDIR /app

COPY include ./include
COPY src ./src
COPY Makefile .

RUN make clean && make && chmod +x url_shortener

EXPOSE 8080

CMD ["sh", "-c", "sleep 10 && /app/url_shortener"]
