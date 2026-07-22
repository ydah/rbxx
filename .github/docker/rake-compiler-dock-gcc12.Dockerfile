FROM ghcr.io/rake-compiler/rake-compiler-dock-image:1.12.0-mri-x86_64-linux@sha256:2f7eabb005362054f71343bce19534a9c6f9d1ed085c66d773bb8fb67935703c

USER root

RUN apt-get update \
    && apt-get install -y --no-install-recommends software-properties-common \
    && add-apt-repository -y ppa:ubuntu-toolchain-r/test \
    && apt-get update \
    && apt-get install -y --no-install-recommends gcc-12 g++-12 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 120 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 120 \
    && rm -rf /var/lib/apt/lists/*
