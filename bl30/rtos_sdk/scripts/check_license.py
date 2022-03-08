#!/usr/bin/python3
#coding:utf-8
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

import os
import sys
import string
import re

#file type contains  .[chS]/.ld
def src_file_check_license(line):
    license =\
'''/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */\n
'''
    if(line.find('\r\n') > 0):
        line_new = re.sub("\r\n", "\n", line)
        return line_new.find(license)
    else:
        return line.find(license)

#file type contains Script/defconfig/prj.conf
def script_file_check_license(line):
    license =\
'''#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#\n
'''
    if(line.find('\r\n') > 0):
        line_new = re.sub("\r\n", "\n", line)
        return line_new.find(license)
    else:
        return line.find(license)

#file type contains Kconfig/CMakeLists.txt
def cfg_file_check_license(line):
    license =\
'''# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT\n
'''
    if(line.find('\r\n') > 0):
        line_new = re.sub("\r\n", "\n", line)
        return line_new.find(license)
    else:
        return line.find(license)

if __name__ == '__main__':
    check_ret = 0
    src_type = ['.c','.h','.S', '.ld']
    cfg_type = ['Kconfig', 'CMakeLists.txt', '.cmake']
    script_type = ['.sh', '.py', '.conf', 'defconfig']
    line = sys.stdin.readline()
    while line:
        if 'diff --git' in line:
            #diff start of file, and get the file name
            file = line.split(' ')[-1].strip()
            file = file.split('/', 1)[1]
            line = sys.stdin.readline()
            patch_str = ''
            if 'new file mode' in line:
                #new file in patch, and save the patch string
                try:
                    for line in sys.stdin:
                        if 'diff --git' in line:
                            break
                        patch_str += line[1:]
                except:
                    print("read patch exception!!")

                if file.split('/')[-1] in cfg_type:
                    ret = cfg_file_check_license(patch_str)
                    if ret < 0:
                        print(file + ' license error\r\n')
                        check_ret = 1
                    continue

                if os.path.splitext(file)[-1] in src_type:
                    ret = src_file_check_license(patch_str)
                    if ret < 0:
                        print(file + ' license error\r\n')
                        check_ret = 1
                    continue

                if os.path.splitext(file)[-1] in script_type:
                    ret = script_file_check_license(patch_str)
                    if ret < 0:
                        print(file + ' license error\r\n')
                        check_ret = 1
                    continue
        line = sys.stdin.readline()

    if check_ret != 0:
        print('\nRTOS Opensource License Check Failed, ')
        print('refs http://tee.amlogic.com:8000/Documents/Ecosystem/RTOS/rtos-sdk/licensing.html')
    else:
        print('\nRTOS Opensource License Check Success\n')