FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y g++ make tzdata && rm -rf /var/lib/apt/lists/*

RUN ln -fs /usr/share/zoneinfo/Asia/Novosibirsk /etc/localtime

WORKDIR /app

COPY *.cpp *.h Makefile ./

RUN make clean && make && chmod +x statistics_service

EXPOSE 8081

CMD ["sh", "-c", "sleep 10 && /app/statistics_service"]
