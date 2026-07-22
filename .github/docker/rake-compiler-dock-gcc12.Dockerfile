FROM ghcr.io/rake-compiler/rake-compiler-dock-image:1.12.0-mri-x86_64-linux@sha256:2f7eabb005362054f71343bce19534a9c6f9d1ed085c66d773bb8fb67935703c

USER root

RUN set -eux; \
    for attempt in 1 2 3 4 5; do \
      if apt-key adv \
        --keyserver hkp://keyserver.ubuntu.com:80 \
        --recv-keys 60C317803A41BA51845E371A1E9377A2BA9EF27F; then \
        break; \
      fi; \
      if [ "$attempt" -eq 5 ]; then \
        exit 1; \
      fi; \
      sleep "$((attempt * 5))"; \
    done; \
    echo "deb http://ppa.launchpad.net/ubuntu-toolchain-r/test/ubuntu focal main" \
      > /etc/apt/sources.list.d/ubuntu-toolchain-r.list; \
    apt-get \
      -o Acquire::Retries=10 \
      -o Acquire::http::Timeout=20 \
      -o Acquire::Queue-Mode=access \
      update; \
    apt-get \
      -o Acquire::Retries=10 \
      -o Acquire::http::Timeout=20 \
      -o Acquire::Queue-Mode=access \
      install -y --no-install-recommends gcc-12 g++-12; \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 120; \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 120; \
    rm -rf /var/lib/apt/lists/*
