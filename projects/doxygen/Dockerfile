FROM ubuntu:18.04


FROM ubuntu:18.04 as builder

ENV USER=wirepas
ENV HOME=/home/wirepas
ENV APP_HOME=${HOME}/app

RUN groupadd -r ${USER} --gid 1000 \
    && useradd --uid 1000 -r -g ${USER} ${USER}

RUN apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    doxygen graphviz nano git\
 && rm -rf /var/lib/apt/lists/*

WORKDIR ${APP_HOME}
RUN chown ${USER}:${USER} -R ${HOME}
USER wirepas

CMD ["doxygen", "docs/Doxyfile"]
