FROM debian:sid-slim

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y --no-install-recommends apt-utils && \
    apt-get install -y --no-install-recommends \
	    libc-dev \
	    cmake make \
	    ccache \
	    git ca-certificates curl file \
	    mesa-utils \
	    libxrandr-dev \
	    libxcursor-dev \
	    libxi-dev \
	    libxinerama-dev \
	    libgl1-mesa-dev \
	    libdwarf-dev binutils-dev \
        g++ \
        clang libc++-dev libc++abi-dev llvm \
        glslang-tools \
	&& apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

RUN git config --global http.sslVerify false

RUN echo "max_size = 1.0G\nsloppiness = file_macro,pch_defines,time_macros" > /etc/ccache.conf
