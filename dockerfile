FROM whenning42/manylinux2014_x86_64_i386
# ^ Dockerfile for this base container at ./docker/manylinux2014_x86_64_i386

ENV PATH="/root/.local/bin:${PATH}"

RUN yum check-update
RUN yum install -y vim gtest-devel glibc-devel.i686 libatomic.i686 devtoolset-10-libstdc++-devel.i686 
RUN pipx install poetry

COPY . /workspace
WORKDIR /workspace
RUN python3.11 -m venv .venv && source .venv/bin/activate && poetry install --no-root && ./build_libs.sh --release
# We don't need to be in the venv to build the package wheels.
RUN poetry build && cd dist && auditwheel repair libtimecontrol*.whl
