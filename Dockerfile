FROM alpine:3.14

COPY . /src

WORKDIR /src

RUN apk add --no-cache libnet-dev libev-dev cmake gcc musl-dev && \
    cmake -B 'build' && \
    cmake --build -B 'build' && \
    ls -R

ENTRYPOINT ["/src/build/server"]
