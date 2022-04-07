#!/usr/bin/python3
# -*- coding: UTF-8 -*-
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

import os
import sys
import getopt
from xml.dom.minidom import parse
import xml.dom.minidom
import xlwt

VERSION="Copyright (c) 2021-2022 Amlogic v2022.4.6"
nm_cmd="./output/toolchains/gcc-aarch64-none-elf/aarch64-none-elf/bin/nm"
nm_para= " -l -S -t d --size-sort "
elf_path=" ./output/arm64-ad403_a113l-speaker/freertos/freertos.elf"
report_file="module_size_report.xls"
manifest_file="./.repo/manifests/default.xml"

def usage():
    """
The script is  generate the module size for freertos.elf,
Usage: ./scripts/module_size_report.py <manifest_file>

Example: ./scripts/module_size_report.py -m ./.repo/manifests/default.xml
Note: This script is depend on xlwt library, install cmd is "pip3 install xlwt"

Description
    -h --help           display help information
    -n <nm_file>        tools path of nm
    -e <elf_file>       file path of freertos.elf
    -m <manifest_file>  file path of manifest.xml
    -o <report_file>    report file of results
    -v --version        version information
"""

if __name__ == '__main__':
    try:
        opts, args = getopt.getopt(sys.argv[1:], "n:e:m:o:hv", ["help","version"])
    except getopt.GetoptError as err:
        print(err)
        print(usage.__doc__)
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            print(usage.__doc__)
            sys.exit()
        elif opt in ("-n"):
            nm_cmd = arg
        elif opt in ("-e"):
            elf_path = arg
        elif opt in ("-m"):
            manifest_file = arg
        elif opt in ("-o"):
            report_file = arg
        elif opt in ("-v", "--version"):
            print(VERSION)
            sys.exit()
        else:
            print("Using the wrong way, please refer the help information!")
            assert False, "unhandled option"

    DOMTree = xml.dom.minidom.parse(manifest_file)
    root = DOMTree.documentElement
    prjs = root.getElementsByTagName("project")
    os.system(nm_cmd+nm_para+elf_path+" > symbols_1.txt")
    book = xlwt.Workbook(encoding='utf-8', style_compression=0)
    sheet = book.add_sheet('symbol_report', cell_overwrite_ok=True)
    col = ['Module', 'Text', 'Data', 'BSS', 'Binary(text+Data)', 'Total']
    for i in range(0,len(col)):
        #write the first row
        sheet.write(0, i, col[i])
    row = 1
    for prj in prjs:
        path=prj.getAttribute("path")
        os.system("grep -w "+path+" symbols_1.txt > symbols_2.txt")
        os.system("grep -E ' [tT] ' symbols_2.txt > symbols_3.txt")
        code_size=os.popen("awk 'BEGIN{size=0;}{size=size+$2;}END{print size;}' symbols_3.txt").read()
        os.system("grep -E ' [dD] ' symbols_2.txt > symbols_3.txt")
        data_size=os.popen("awk 'BEGIN{size=0;}{size=size+$2;}END{print size;}' symbols_3.txt").read()
        os.system("grep -E ' [bB] ' symbols_2.txt > symbols_3.txt")
        bss_size=os.popen("awk 'BEGIN{size=0;}{size=size+$2;}END{print size;}' symbols_3.txt").read()
        #print(code_size + data_size + bss_size)
        total_size=int(code_size)+int(data_size)+int(bss_size)
        #print(total_size)
        if(total_size!=0):
            sheet.write(row, 0, path)
            sheet.write(row, 1, int(code_size))
            sheet.write(row, 2, int(data_size))
            sheet.write(row, 3, int(bss_size))
            sheet.write(row, 4, int(code_size)+int(data_size))
            sheet.write(row, 5, int(total_size))
            row +=1

#write last row
sheet.write(row, 0, "Sum")
sheet.write(row, 1, xlwt.Formula('SUM(B2:B%d'%(row)+')'))
sheet.write(row, 2, xlwt.Formula('SUM(C2:C%d'%(row)+')'))
sheet.write(row, 3, xlwt.Formula('SUM(D2:D%d'%(row)+')'))
sheet.write(row, 4, xlwt.Formula('SUM(E2:E%d'%(row)+')'))
sheet.write(row, 5, xlwt.Formula('SUM(F2:F%d'%(row)+')'))
book.save(report_file)
os.popen('rm symbols_1.txt symbols_2.txt symbols_3.txt')