# syntax=docker/dockerfile:1.27

FROM ghcr.io/unity-desktop/builder

SHELL ["/bin/bash", "-euxo", "pipefail", "-c"]

# hadolint ignore=DL3008
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt/lists,sharing=locked \
    --mount=type=tmpfs,target=/var/log <<-EOT
    apt-get update
    apt-get install -y --no-install-recommends \
        meson ninja-build gi-docgen gobject-introspection \
        libglib2.0-dev gir1.2-glib-2.0-dev libgirepository-2.0-dev \
        libgtk-4-dev libadwaita-1-dev libgtk4-layer-shell-dev libastal-4-dev \
        libgsound-dev libgnome-desktop-4-dev libgnome-bg-4-dev \
        gsettings-desktop-schemas-dev libpng-dev
EOT
