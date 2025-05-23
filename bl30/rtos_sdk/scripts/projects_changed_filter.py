#!/usr/bin/python3
#coding:utf-8
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

import sys
import os
import argparse
import subprocess

def is_subpath(path1, path2):
    common_part = os.path.commonpath([path1, path2])
    return common_part == path1

class project_c:
    def __init__(self, full_name):
        self.full_name = full_name
        elements = full_name.split(' ')
        self.arch = elements[0]
        self.soc = elements[1]
        self.board = elements[2]
        self.product = elements[3]
        self.repository_set = set()

    def bind_repository_set(repository_set):
        self.repository_set = repository_set

class repository_set_c:
    def __init__(self):
        self.container_set = set()

    def add_repository(self, repository_pair):
        self.container_set.add(repository_pair)

    def get_repository_name(self, bind_dir_path):
        repository_name = ""

        for repository in self.container_set:
            if is_subpath(repository[1], bind_dir_path):
                repository_name = repository[0]
        return repository_name

    def get_repository_path(self, bind_rep_name):
        repository_path = ""

        for repository in self.container_set:
            if bind_rep_name == repository[0]:
                repository_path = repository[1]
        return repository_path

def main():
    parser = argparse.ArgumentParser(description='Filter the build project which is changed.')
    parser.add_argument('output_path', help="The project output dir path.")
    parser.add_argument('prj_msg', help="The project description line message")
    parser.add_argument('changed_reps', help="The changed repository list")
    args = parser.parse_args()
    output_path = args.output_path
    prj_msg = args.prj_msg
    changed_reps = args.changed_reps

    prj_elements = prj_msg.split(' ')
    arch = prj_elements[0]
    soc = prj_elements[1]
    board = prj_elements[2]
    product = prj_elements[3]
    project_path = output_path + "/{}-{}-{}".format(arch, board, product)
    base_path = os.path.abspath(output_path + "/..")

    project_list_file = output_path + "/build_combination.txt"
    project_list_file = os.path.abspath(project_list_file)
    with open(project_list_file) as f:
        lines = iter(f)
        for line in lines:
            project = project_c(line)

    repository_list_file = project_path + "/freertos/rtos_sdk_manifest.xml"
    all_repository_set = repository_set_c()
    with open(repository_list_file) as f:
        lines = iter(f)
        for line in lines:
            line = line.strip()
            if line.find("project name=") < 0:
                continue
            else:
                elements = line.split(' ')
                name_str = elements[1].split('=')[1][1:-1]
                path_str = elements[2].split('=')[1][1:-3]
                repository = (name_str, path_str)
                all_repository_set.add_repository(repository)

    dirs_file = project_path + "/freertos/build.ninja"
    prj_repository_set = repository_set_c()
    prj_repository_unique_set = set()
    with open(dirs_file) as f:
        lines = iter(f)
        for line in lines:
            line = line.strip()
            if "DEP_FILE =" in line:
                keyword = "obj/"
                index_left = line.find(keyword)
                if (index_left < 0):
                    continue
                else:
                    index_left = index_left + len(keyword)
                keyword = "/CMakeFiles/"
                index_right = line.find(keyword)
                if (index_right < 0) or (index_left > index_right):
                    continue
                else:
                    module_dir = line[index_left : index_right]
                    if (len(module_dir) == 0):
                        continue
                    name_str = all_repository_set.get_repository_name(module_dir)
                    if (len(name_str) > 0):
                        prj_repository_set.add_repository((name_str, module_dir))
                        prj_repository_unique_set.add(name_str)
    prj_repository_unique_set.add("rtos_sdk/build")
    prj_repository_unique_set.add("rtos_sdk/scripts")
    prj_repository_unique_set.add("rtos_sdk/product/" + os.environ.get('PRODUCT'))
    changed_reps_set = changed_reps.split('#')
    changed_flag = 0
    for changes_rep in changed_reps_set:
        if changes_rep in prj_repository_unique_set:
            changed_flag = 1
            break

    return changed_flag

if __name__ == "__main__":
    sys.exit(main())
