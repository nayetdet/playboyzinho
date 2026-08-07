ARG VARIANT=3.12-bookworm
FROM mcr.microsoft.com/devcontainers/python:${VARIANT}
ENV DEBIAN_FRONTEND=noninteractive \
    PATH="/home/vscode/.local/bin:${PATH}"

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        clang \
        udev \
    && rm -rf /var/lib/apt/lists/*

## Set up udev rules for PlatformIO
RUN curl -fsSL \
    https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/system/99-platformio-udev.rules \
    -o /etc/udev/rules.d/99-platformio-udev.rules

RUN usermod -aG dialout,plugdev vscode

### Set up compatibility with Fedora host (if needed)
### Since Fedora uses `18` as the group for dialout, we need to add it to the container
ARG FEDORA_COMPAT=0
RUN if [ "$FEDORA_COMPAT" = "1" ]; then \
  sudo groupadd -g 18 compat_dialout; \
  sudo usermod -a -G compat_dialout vscode; \
fi

# Install PlatformIO CLI
USER vscode
RUN python3 -m pip install --user --no-cache-dir platformio
RUN pio system info
WORKDIR /workspaces
