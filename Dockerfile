FROM alpine

RUN apk add --no-cache git
RUN apk add --no-cache g++
RUN apk add --no-cache clang
RUN apk add --no-cache make
RUN apk add --no-cache doxygen
RUN apk add --no-cache cppcheck
RUN apk add --no-cache sqlite-dev
RUN apk add --no-cache openssl-dev
RUN apk add --no-cache zlib-dev
RUN apk add --no-cache compiler-rt-static
RUN mkdir -p /usr/lib/clang/9.0.0/lib/linux
RUN ln -s /usr/lib/clang/9.0.0/libclang_rt.profile-x86_64.a /usr/lib/clang/9.0.0/lib/linux/libclang_rt.profile-x86_64.a

RUN git clone https://github.com/marcpage/os.git
RUN git clone https://github.com/marcpage/protocol.git
RUN git clone https://github.com/marcpage/libernet.git
