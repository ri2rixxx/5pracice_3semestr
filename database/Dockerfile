FROM alpine:latest

RUN apk add --no-cache g++ make netcat-openbsd

WORKDIR /app

COPY src/ ./src/
COPY include/ ./include/

RUN mkdir -p src/structures

RUN g++ -std=c++17 -I./include -I./src \
    src/main.cpp \
    src/server.cpp \
    src/command_processor.cpp \
    src/simple_commands.cpp \
    src/database_extended.cpp \
    src/structures/stack.cpp \
    src/structures/set.cpp \
    src/structures/hash_table.cpp \
    src/structures/queue.cpp \
    -pthread -o server

EXPOSE 6379

RUN mkdir -p /data

VOLUME /data

CMD ["./server"]
