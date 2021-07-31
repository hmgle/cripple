FROM alpine:3.14

COPY . /src

WORKDIR /src/build

RUN apk add --no-cache libnet-dev libev-dev cmake make pkgconf gcc musl-dev
RUN    cmake .. && \
    cmake --build . && \
    ls -R

ENTRYPOINT ["/src/build/server"]
