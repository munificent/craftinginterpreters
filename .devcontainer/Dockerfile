FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    openjdk-17-jdk \
    make build-essential \
    curl unzip sudo \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Download and install Dart SDK 2.12.0 (Linux x64) with appropriate permissions
RUN curl -L -o dart-sdk.zip https://storage.googleapis.com/dart-archive/channels/stable/release/2.12.0/sdk/dartsdk-linux-x64-release.zip \
    && unzip dart-sdk.zip -d /usr/local/ \
    && rm dart-sdk.zip \
    && chmod -R a+rx /usr/local/dart-sdk

ENV PATH="/usr/local/dart-sdk/bin:${PATH}"

# Create a non-root user (vscode) with sudo and no password
RUN useradd -ms /bin/bash vscode && echo "vscode ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

USER vscode
WORKDIR /workspace