FROM debian:jessie
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends build-essential libglfw3-dev libglew-dev pkg-config libxrandr-dev libxi-dev
ADD . /usr/src/imgui
WORKDIR /usr/src/imgui
RUN make -C examples/opengl_example all
CMD ["/usr/src/imgui/examples/opengl_example/imgui_example"]
