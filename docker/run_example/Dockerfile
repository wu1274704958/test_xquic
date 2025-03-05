# syntax=docker/dockerfile:1

FROM ubuntu:24.04

#RUN sed -i "s@http://.*archive.ubuntu.com@https://mirrors.tuna.tsinghua.edu.cn@g" /etc/apt/sources.list
#RUN sed -i "s@http://.*security.ubuntu.com@https://mirrors.tuna.tsinghua.edu.cn@g" /etc/apt/sources.list
RUN apt-get update -y
RUN apt upgrade -y
RUN apt-get install -y --fix-missing libc++-14-dev
RUN apt-get install -y --fix-missing openssh-server git clang gdb libstdc++-13-dev
RUN apt-get install -y --fix-missing curl zip unzip cmake
RUN apt-get install -y --fix-missing pkg-config build-essential tar
RUN apt-get install -y --fix-missing nasm golang-go
#RUN echo "root:112" | chpasswd
WORKDIR /root
RUN mkdir code
#COPY ../../../test_xquic /root/code/
WORKDIR /root/code
RUN git clone --branch 2023.10.19 https://github.com/microsoft/vcpkg.git
RUN ./vcpkg/bootstrap-vcpkg.sh
RUN ./vcpkg/vcpkg install zlib
RUN ./vcpkg/vcpkg install boringssl
RUN ./vcpkg/vcpkg install liblsquic
RUN ./vcpkg/vcpkg install libuv
RUN ./vcpkg/vcpkg install toml11
RUN ./vcpkg/vcpkg install easyloggingpp
RUN ./vcpkg/vcpkg install protobuf
RUN ./vcpkg/vcpkg install libsigcpp
#RUN ./vcpkg/vcpkg install pdcurses
RUN ./vcpkg/vcpkg integrate install
RUN apt remove -y gcc g++



