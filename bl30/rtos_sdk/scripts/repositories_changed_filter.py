#!/usr/bin/python3
#coding:utf-8
#
# Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT
#

import subprocess

def split_into_sections(content):
    # Split the content by empty lines
    paragraphs = content.split("\n\n")

    # Initialize variables
    sections = []
    current_section = []

    # Iterate over each paragraph
    for paragraph in paragraphs:
        lines = paragraph.split("\n")
        if len(lines) >= 2 and lines[0].startswith("project") and lines[1].startswith("On branch"):
            # Start a new section
            if current_section:
                sections.append(current_section)
            current_section = [paragraph]
        else:
            # Add to the current section
            current_section.append(paragraph)

    # Add the last section
    if current_section:
        sections.append(current_section)

    return sections

shell_command = "repo forall -p -c git status"
result = subprocess.run(shell_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
content = result.stdout

sections = split_into_sections(content)
changed_repository_list = ""
for p in sections:
    if p[0].find("is up to date with") < 0 and (p[0].find("git push") > 0 or p[0].find("different commits")) > 0:
        reps_name = "rtos_sdk/" + p[0][0 + len("project") + 1 : p[0].find('\n') - 1]
        changed_repository_list = changed_repository_list + reps_name + "#"
print(changed_repository_list)