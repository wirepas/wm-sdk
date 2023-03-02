# Wirepas Oy
#!/usr/bin/env bash

MAKE_BUILD=${1:-"false"}

function _create_pdf
{
    APP_NAME=${1:-"$(basename $(dirname $(pwd)))"}

    LATEX_PATH=${2:-"latex"}
    PDF_NAME=${3:-"refman.pdf"}

    cd ${LATEX_PATH}
    make
    cd ..
    echo "${LATEX_PATH}/${PDF_NAME} ${APP_NAME}.pdf"
    cp ${LATEX_PATH}/${PDF_NAME} ${APP_NAME}.pdf
}

function _clean_up
{
    LATEX_PATH=${1:-"latex"}
    HTML_PATH=${2:-"html"}

    rm -rf ${LATEX_PATH}
    rm -rf ${HTML_PATH}/*.map
    rm -rf ${HTML_PATH}/*.md5
}


function _apply_wirepas_template
{

    DOCUMENT_TITLE=${1:-"$(basename $(dirname $(pwd))|tr '_' ' ' )"}
    DOCUMENT_VERSION=${2:-"4.0.0A"}
    DOCUMENT_DESCRIPTION=${3:-"Wirepas Application Interface"}

    SOURCE_PATH="latex"

    END="%--- End generated contents ---"
    START="%--- Begin generated contents ---"
    SECTIONS=$(sed -n -e "/^${START}$/,/^${END}$/{ /^${START}$/d; /^${END}$/d; p; }" ${SOURCE_PATH}/refman.tex)

    TEMPLATE_PATH="template"

    cp ${TEMPLATE_PATH}/* ${SOURCE_PATH}/

    for LINE in ${SECTIONS}
    do
        echo ${LINE} >> ${SOURCE_PATH}/doc.tex
    done

    cat >>${SOURCE_PATH}/doc.tex <<EOL
     % Index
    \newpage
    \phantomsection
    \clearemptydoublepage
    \addcontentsline{toc}{section}{Index}
    \printindex
    \end{document}
EOL

    sed -i "s|#DOCUMENT_VERSION|${DOCUMENT_VERSION}|g" ${SOURCE_PATH}/doc.tex
    sed -i "s|#DOCUMENT_TITLE|${DOCUMENT_TITLE^}|g" ${SOURCE_PATH}/doc.tex
    sed -i "s|#DOCUMENT_DESCRIPTION|${DOCUMENT_DESCRIPTION}|g" ${SOURCE_PATH}/doc.tex

    cp ${SOURCE_PATH}/doc.tex ${SOURCE_PATH}/refman.tex

}


function _main
{
    dir=$(pwd)
    parentdir="$(dirname "$dir")"

    if [ ${MAKE_BUILD} == true ];
    then
        docker build --compress -t wirepas-doxygen .
    fi

    docker run --rm \
               --user $(id -u):$(id -g) \
                -v ${parentdir}/:/home/wirepas/app/ \
                 wirepas-doxygen \
                bash \
                -c "doxygen docs/Doxyfile.template"

    _apply_wirepas_template
    _create_pdf
    _clean_up
}


_main "${@}"

