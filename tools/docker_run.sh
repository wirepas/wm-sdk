#!/bin/bash

#Docker Development Helper v.01
#Start docker image to be attached with VSCode remote container extension

#Enforce correct directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
# Change to parent folder of this script
cd $DIR/..

FILE=bitbucket-pipelines.yml

if [ -f $FILE ]; then
   echo "File $FILE exists."
else
   echo "File $FILE does not exist."
   exit
fi

IMAGE=$(grep image ${FILE} | head -1 | tr -d "[:space:]" | cut -f 2- -d ':')

#Note the directory structure is same as on host to avoid problems with source paths in debugger!
docker run --rm -it -v $(pwd)/..:$(pwd)/.. -w $(pwd) ${IMAGE}
