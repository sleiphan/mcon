FROM debian:trixie AS base

# Install necessary dependencies for setup
RUN apt update && apt install -y curl gnupg

# Add my personal package repository
RUN echo "deb [signed-by=/usr/share/keyrings/sleiphan.gpg] https://sleiphan.github.io/debian-repo stable main" | tee /etc/apt/sources.list.d/sleiphan.list

# Add my package repository's public gpg key
RUN curl -fsSL https://sleiphan.github.io/debian-repo/public.key | gpg --dearmor -o /usr/share/keyrings/sleiphan.gpg

# Install dependencies
RUN apt update && apt install -y \
    git \
    gh \
    build-essential \
    gdb \
    clang-format \
    cmake \
    man-db \
    liburing-dev \
    libctools-dev

# Set locale to UTF-8
ENV LC_ALL='C.utf8'
