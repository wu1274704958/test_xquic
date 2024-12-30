# syntax=docker/dockerfile:1

FROM ubuntu:24.04

#RUN sed -i "s@http://.*archive.ubuntu.com@https://mirrors.tuna.tsinghua.edu.cn@g" /etc/apt/sources.list
#RUN sed -i "s@http://.*security.ubuntu.com@https://mirrors.tuna.tsinghua.edu.cn@g" /etc/apt/sources.list
RUN apt-get update
RUN apt upgrade
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

CMD ["bash", "-c", "cd /root/code/test_xquic && rm -rf build-linux && mkdir build-linux && cd build-linux && cmake -DBUILD_MQAS_EXAMPLES=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/root/code/vcpkg/scripts/buildsystems/vcpkg.cmake .. && make test_p2p_ser && cd examples/test_p2p_ser/ && ./test_p2p_ser"]

EXPOSE 27 8084



