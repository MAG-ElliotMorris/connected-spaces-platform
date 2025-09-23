# Use the official emscripten image as a base
ARG EMSDK_TAG=3.1.57
FROM emscripten/emsdk:${EMSDK_TAG}

# Switch into the emscripten source dir
WORKDIR /emsdk/upstream/emscripten

# Install TypeScript + other JS deps (needed for --embind-emit-tsd)
RUN npm ci || npm install

# Default workdir when you run the container
WORKDIR /work

# Make sure temporary files are kept and debuggable
ENV EMCC_DEBUG=1