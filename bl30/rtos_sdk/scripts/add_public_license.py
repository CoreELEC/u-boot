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

def add_Header_01(filepath, filename):
    if os.path.exists(filepath) :
        head_info = string.Template(
'''/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */\n
''')
        head = head_info.substitute(vars())
        f = open(filepath, "r", errors='ignore', newline='')
        content = f.read()
        f_w = open(filepath, "w", errors='ignore', newline='')
        f_w.seek(0,0)

        if(content.find('\r\n') > 0):
            head_new = re.sub("\n", "\r\n", head)
            f_w.write(head_new)
        else:
            f_w.write(head)

        f_w.write(content)
        f_w.close


def add_Header_02(filepath, filename):
    if os.path.exists(filepath) :
        head_info = string.Template(
'''# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.

# SPDX-License-Identifier: MIT\n
''')
        head = head_info.substitute(vars())
        f = open(filepath, "r", errors='ignore', newline='')
        content = f.read()
        f_w = open(filepath, "w", errors='ignore', newline='')
        f_w.seek(0,0)

        if(content.find('\r\n') > 0):
            head_new = re.sub("\n", "\r\n", head)
            f_w.write(head_new)
        else:
            f_w.write(head)

        f_w.write(content)
        f_w.close

def add_Header_03(filepath, filename):
    if os.path.exists(filepath) :
        head_info = string.Template(
'''#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#\n
''')
        head = head_info.substitute(vars())
        f = open(filepath, "r", errors='ignore', newline='')
        content = f.read()
        f_w = open(filepath, "w", errors='ignore', newline='')
        f_w.seek(0,0)

        if(content.find('\r\n') > 0):
            head_new = re.sub("\n", "\r\n", head)
            f_w.write(head_new)
        else:
            f_w.write(head)

        f_w.write(content)
        f_w.close

def del_C_Header(filepath):
    if os.path.exists(filepath) :
        file = open(filepath, "r", errors='ignore', newline='')
        lines = file.readlines()
        beforeTag = True
        writer = open(filepath, 'w', errors='ignore', newline='')
        for line in lines :
            if '#include' in line:
                beforeTag = False
            if beforeTag == False:
                writer.write(line)

def del_H_Header(filepath):
    if os.path.exists(filepath):
        file = open(filepath, "r", errors='ignore', newline='')
        lines = file.readlines()
        beforeTag = True
        writer = open(filepath, 'w', errors='ignore', newline='')
        for line in lines :
            if '#ifndef ' in line or '#include ' in line or '#define ' in line or '#ifdef' in line:
                beforeTag = False
            if beforeTag == False:
                writer.write(line)

if __name__ == '__main__':
    path=sys.argv[1]
    list = os.walk(path, True)
    for dir in list:
        files = dir[2]
        for file in files:
            filepath = os.path.join(dir[0], file)
            print(filepath)
            if 'Kconfig' in file or 'CMakeList' in file:
                add_Header_02(filepath, file)
            elif 'defconfig' in file or 'prj.conf' in file:
                add_Header_03(filepath, file)
            elif '.h' in file or '.ld' in file:
                del_H_Header(filepath)
                add_Header_01(filepath, file)
            elif '.c' in file:
                del_C_Header(filepath)
                add_Header_01(filepath, file)

    print( 'Complete!!!!!!!!!!!!!!!')
