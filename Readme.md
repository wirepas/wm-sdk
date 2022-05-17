# Wirepas SDK

This repository contains Wirepas SDK, which allows the development of an application
to be executed on the same chip as Wirepas Stack.
This application is often referred as a _Single-MCU application_.


> :warning:
>
> _To use the SDK, you need to have access to the Wirepas binaries. You need to have a
> software license agreement (SLA) with Wirepas to get them. If you would like to become
> a SLA licensee, please see the right contact from www.wirepas.com/contact_
>

- [Overview](#overview)
- [Documentation](#documentation)
- [Wirepas binaries](#wirepas-binaries)
- [Environment](#environment)
- [How to build an application](#how-to-build-an-application)
- [License](#license)


## Overview

The following diagram, describes the main components of the SDK.

![Main components][here_main_components]


## Documentation

The documentation for this SDK is written with Doxygen and generated in HTML format.

It is hosted [here](https://wirepas.github.io/wm-sdk/).
You can select the desired version depending on the SDK version you are working on.

Some information is available on this page too, but it is just a subset of what the html documentation
contains.

## Wirepas binaries

As a Wirepas SLA licensee, you should have received access to protected zipped archive containing the Wirepas binaries.
Please extract them at the root of [image folder][here_image] (All the *.a, *.hex and *.conf files must be at the root of this folder).

## Environment

This SDK relies on GNU Arm toolchain. To use the SDK you will need to fulfill the following requirements:

1. A GCC Arm toolchain (_version 10.2.1 is recommended_)
2. The make tool
3. python 3.x
4. pycryptodome package for python (_can be installed with pip_)

In order to validate that your environment is correctly configured, you should be able to build the custom_app application.

For more information, please refer to [Documentation](#documentation)

## How to build an application

This SDK supports multiple target boards. All of them are listed under [board folder][here_board] and can be selected with target_board=<target_board>

This SDK contains multiple application examples that can be found under [source folder][here_source] and can be selected with app_name=<app_name>

> :warning:
>
> The first time you'll build an application, you'll be prompted to choose bootloader keys.
> Once chosen the first time, they will be used for all your images and must be kept secret
> and in a safe place where they will not be lost or deleted.  It is also possible to define
> keys per application.

To build the _custom_app_ application for _pca10040_ board, please execute following command.

```shell
    make app_name=custom_app target_board=pca10040
```


After execution of this command, you should find the _final_image_custom_app.hex_ under _build/pca10040/custom_app_ folder.

For more information, please refer to [Documentation](#documentation)

## License

See [LICENSE][here_license] for full license details.

[here_license]: LICENSE.txt
[here_main_components]: projects/doxygen/media/main_components.png
[here_board]: board/
[here_source]: source/
[here_image]: image/

