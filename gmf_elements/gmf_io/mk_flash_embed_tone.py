#!/user/bin/env python

#  ESPRESSIF MIT License
#
#  Copyright (c) 2022 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
#
#  Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
#  it is free of charge, to any person obtaining a copy of this software and associated
#  documentation files (the "Software"), to deal in the Software without restriction, including
#  without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the Software is furnished
#  to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in all copies or
#  substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
#  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
#  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import sys
import os
import time
import argparse

LICENSE_STR = '''
/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2025 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

'''

GEN_HEAD_FILE_NAME =  'esp_embed_tone.h'
GEN_CMAKE_FILE_NAME = 'esp_embed_tone.cmake'

def sanitize_filename(filename):
    return filename.replace('-', '_')

def gen_h_file(file_dic):
    h_file = '#pragma once\r\n\r\n'
    h_file += '/**\n * @brief Structure for embedding tone information\n */\n'
    h_file += 'typedef struct {\r\n    const uint8_t * address; /**< Pointer to the embedded tone data */\r\n    int size; /**< Size of the tone data in bytes */\r\n} esp_embed_tone_t;\r\n\r\n'
    cmake_file = 'set(COMPONENT_EMBED_TXTFILES'

    enum_file = '\n/**\n * @brief Enumeration for tone URLs\n */\n'
    enum_file += 'enum esp_embed_tone_index {'
    struct_file = '/**\n * @brief Array of tone URLs\n */\n'
    struct_file += 'const char * esp_embed_tone_url[] = {'
    next_h_file = '/**\n * @brief Array of embedded tone information, use in `esp_gmf_io_embed_flash_set_context`\n */\n'
    next_h_file += 'esp_embed_tone_t g_esp_embed_tone[] = {'
    file_num = 0

    for key in file_dic:
        cmake_file += ' ' + key
        sanitized_key = sanitize_filename(key.replace('.', '_'))
        h_file += '/**\n * @brief External reference to embedded tone data\n */\n'
        h_file += 'extern const uint8_t ' + sanitized_key + "[] asm(\"_binary_" + sanitized_key + "_start\");\r\n\r\n"
        str_file_num = str(file_num)
        str_file_size = str(file_dic[key])
        next_h_file += '\n    [' + str_file_num + '] = {\n        .address = ' + sanitized_key + ', /**< Tone data address */\n        .size    = '+ str_file_size +', /**< Tone data size */\n    },'
        file_num += 1

        enum_file += '\n    ESP_EMBED_TONE_' + sanitized_key.upper() + ' = ' + str_file_num + ','
        result = sanitized_key.rfind('_')
        list_key = list(sanitized_key)
        list_key[result] = '.'
        sanitized_key = ''.join(list_key)
        struct_file += "\n    \"" + 'embed://tone/' + str_file_num + '_' + sanitized_key + "\","

    enum_file += '\n    ESP_EMBED_TONE_URL_MAX = ' + str(file_num) + ''
    next_h_file += '\n};\n';
    cmake_file += ')'
    enum_file += '\n};\n\n';
    struct_file += '\n};\n';
    h_file += next_h_file
    h_file += enum_file
    h_file += struct_file
    return h_file, cmake_file

def write_h_file(file, file_name, path):
    if os.path.exists(file_name):
        os.remove(file_name)

    FileName = path + '/' + file_name
    with open(FileName,'w+') as f:
        if f != None:
            if GEN_HEAD_FILE_NAME == file_name:
                f.write(LICENSE_STR)
            f.write(file)
            f.close()

def write_cmake_file(file, path):
    ComponentName = path + '/' + GEN_CMAKE_FILE_NAME
    with open(ComponentName,'w+') as f:
        if f != None:
            f.truncate(0)
            f.write(file)
            f.close()

if __name__ == '__main__':
    argparser = argparse.ArgumentParser()
    argparser.add_argument('-p', '--path', type=str, required=False ,help='base folder for the source files generated')
    args = argparser.parse_args()

    file_list = [x for x in os.listdir(args.path) if ((x.endswith('.wav')) or (x.endswith('.mp3')))]
    file_list.sort()

    print('The file list:\r\n')
    dict_file = dict()
    for i in file_list:
        size = os.path.getsize(i)
        dict_file[i] = size
    for j in dict_file:
        print('name: %s' % j)
        print('size: %d' % dict_file[j])

    h_context, cmake_context  = gen_h_file(dict_file)
    write_h_file(h_context, GEN_HEAD_FILE_NAME, args.path)
    write_cmake_file(cmake_context, args.path)
