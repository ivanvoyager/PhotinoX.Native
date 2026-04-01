FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libgtk-3-dev \
    libwebkit2gtk-4.1-dev \
    libnotify-dev \
    libx11-dev \
    libxkbcommon-dev \
    libayatana-appindicator3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src