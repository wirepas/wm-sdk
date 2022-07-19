# Shared app config library

## Overview

This wrapper allows the usage of the Wirepas app config message in the same network between multiple applications and multiple independent modules inside the same application.
This library will register for app config through the single MCU API and will dispatch it to the registered module based on the subscription and following format.

## Format

The App config format chosen is TLV (Type-Length-Value).
Each app/module can register for one or more types.

Type is two bytes but the encoding is optimized to take only one byte if type <= 255.

### Header
To be parsed by this library, app config should start with the following header 0xF67E (2 bytes) followed by the number of TLV entry on 1 byte

> header | tlv-number | type-1 | length-1 | value-1 | ... | type-n | length-n | value-n


In case of an app config starting with a different header, its content is not parsed by the library
and sent to registered module directly (module who registered to type SHARED_APP_CONFIG_INCOMPATIBLE_FILTER and SHARED_APP_CONFIG_ALL_TYPE_FILTER).

### TLV encoding
As length available in app config is relatively short, the length only require 7 bytes. The MSB of the Length field is used to improve the Type encoding.
If Type <= 255, Type field is encoded in one byte and Length MSB is set to 0.
If type > 255, Type field is encoded on 2 bytes and the Length MSB is set to 1. In this case, the encoding is T(Lower part)L(With MSB = 1)T(higher part).

### Example
In this example, the app config contains 2 TLV fields as followed:

Type          | Length        | Value
------------- | ------------- | -------------
0x10          | 2             | 0x0102
0x1234        | 4             | 0xAABBCCDD

It will result in following encoding:

Byte(s)       | Value         | Comment
------------- | ------------- | -------------
0             | 0xF6          | App config header byte 0
1             | 0x7E          | App config header byte 1
2             | 0x02          | Number of TLV entry
3             | 0x10          | Entry one type
4             | 0x02          | Entry one Length
5-6           | 0x0102        | Entry one Value
7             | 0x34          | Entry two LSB type
8             | 0x84          | Entry two Length (Most significant bit set to 1)
9             | 0x12          | Entry two MSB type
10-13         | 0xAABBCCDD    | Entry two Value

## Type allocation

For now type allocation is not managed by Wirepas in the ecosystem but some type values are reserved for Wirepas usage:

Values Dec    | Values Hex      | Comment
------------- | -------------   | -------------
0-191         | 0x00 - 0xBF     | Short Type: Free to be used
192-255       | 0xC0 - 0xFF     | Short Type: Reserved for Wirepas
256-61439     | 0xFF - 0xEFFF   | Long Type: Free to be used
61440-65533   | 0xF000 - 0xFFFD | Long Type: Reserved for Wirepas
65533         | 0xFFFE          | Special value for raw app config (not following this format)
65535         | 0xFFFF          | Special value for all types



