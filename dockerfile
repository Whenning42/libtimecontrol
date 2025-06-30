# A proof-of-concept docker container for building 32-bit and 64-bit together in a 
# manylinux container.

FROM quay.io/pypa/manylinux2014_x86_64

ENV PATH="/root/.local/bin:${PATH}"

# We pull the 32-bit libraries repo from manylinux's implementation so we don't have any
# guarantees that it'll stay working in the future.
# https://github.com/pypa/manylinux/blob/fa343998d210b94cf5e27ec9f8c3a23d239043de/docker/build_scripts/install-runtime-packages.sh#L115
RUN curl -fsSLo /etc/yum.repos.d/mayeut-devtoolset-10.repo https://copr.fedorainfracloud.org/coprs/mayeut/devtoolset-10/repo/custom-1/mayeut-devtoolset-10-custom-1.repo
RUN sed -i 's/\$basearch/i386/g' /etc/yum.repos.d/mayeut-devtoolset-10.repo 
RUN yum check-update

RUN yum install -y vim gtest-devel glibc-devel.i686 libatomic.i686 devtoolset-10-libstdc++-devel.i686 

COPY . /workspace
WORKDIR /workspace
RUN pipx install poetry
RUN python3.11 -m venv .venv && source .venv/bin/activate && poetry install --no-root && ./build.sh
