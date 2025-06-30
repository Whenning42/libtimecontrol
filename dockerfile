FROM whenning42/manylinux2014_x86_64_i386

ENV PATH="/root/.local/bin:${PATH}"

RUN yum check-update
RUN yum install -y vim gtest-devel glibc-devel.i686 libatomic.i686 devtoolset-10-libstdc++-devel.i686 

COPY . /workspace
WORKDIR /workspace
RUN pipx install poetry
RUN python3.11 -m venv .venv && source .venv/bin/activate && poetry install --no-root && ./build.sh
