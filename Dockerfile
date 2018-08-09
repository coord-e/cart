FROM alpine:edge

RUN echo '@testing http://dl-cdn.alpinelinux.org/alpine/edge/testing' >> /etc/apk/repositories

RUN apk update && apk upgrade

RUN apk add --update --no-cache \
      libtbb@testing \
      libtbb-dev@testing \
      clang-dev \
      libjpeg \
      openblas \
      libdc1394 \
      libpng \
      jasper \
      tiff \
      libwebp
RUN apk add --update --no-cache \
      --virtual .build-deps \
      build-base \
      openblas-dev \
      unzip \
      wget \
      cmake \
      libdc1394-dev \
      # accelerated baseline JPEG compression and decompression library
      libjpeg-turbo-dev \
      # Portable Network Graphics library
      libpng-dev \
      # A software-based implementation of the codec specified in the emerging JPEG-2000 Part-1 standard (development files)
      jasper-dev \
      # Provides support for the Tag Image File Format or TIFF (development files)
      tiff-dev \
      # Libraries for working with WebP images (development files)
      libwebp-dev \
      linux-headers


ENV CC /usr/bin/clang
ENV CXX /usr/bin/clang++

RUN mkdir -p /tmp && cd /tmp && \
    wget https://github.com/opencv/opencv/archive/3.3.0.zip && \
    unzip 3.3.0.zip
RUN cd /tmp/opencv-3.3.0 && \
    mkdir build && \
    cd build && \
    cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local -D WITH_FFMPEG=NO -D WITH_PYTHON=NO \
      -D WITH_IPP=NO -D WITH_OPENEXR=NO .. && \
    make VERBOSE=1 && \
    make && \
    make install

COPY . /cart-build
RUN cd /cart-build && mkdir build && cd build \
    && cmake .. \
    && make -j"$(nproc)" \
    && make install

RUN rm -rf /cart-build \
    && rm -rf /tmp/* \
    && apk del --purge .build-deps \
    && rm -rf /var/cache/apk/*
