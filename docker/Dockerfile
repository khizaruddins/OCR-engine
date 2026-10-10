# ==============================================================================
# Stage 1: Build Environment
# ==============================================================================
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    pkg-config \
    libtesseract-dev \
    libleptonica-dev \
    libpoppler-cpp-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . /workspace

RUN cmake -S /workspace -B /workspace/build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON \
    -DBUILD_BENCHMARKS=ON \
    -DBUILD_EXAMPLES=ON

RUN cmake --build /workspace/build -j$(nproc)

# ==============================================================================
# Stage 2: Minimal Production Runtime
# ==============================================================================
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
ENV TESSDATA_PREFIX=/usr/share/tesseract-ocr/4.00/tessdata

RUN apt-get update && apt-get install -y --no-install-recommends \
    libtesseract4 \
    tesseract-ocr-eng \
    tesseract-ocr-osd \
    libleptonica-dev \
    libpoppler-cpp0v5 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy binaries and configurations
COPY --from=builder /workspace/build/apps/edmo-ocr/edmo-ocr /usr/local/bin/edmo-ocr
COPY --from=builder /workspace/build/benchmarks/edmo_ocr_benchmark /usr/local/bin/edmo_ocr_benchmark
COPY --from=builder /workspace/configs /app/configs

ENTRYPOINT ["/usr/local/bin/edmo-ocr"]
CMD ["--help"]
