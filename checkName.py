#!/bin/env python3

import re
import sys

name = '???'
matrnr = '???'
with open('student_info.txt') as info:
    for line in info:
        if line.lstrip()[0] == '#':
            continue
        maybeName = re.match('.*Name:\s*(.*)', line, re.IGNORECASE)
        if maybeName is not None:
            name = maybeName.group(1).strip()
        for pattern in { '.*Matriculation number:\s*(.*)', '.*Matrikelnummer:\s*(.*)', '.*MatrNr:\s*(.*)'}:
            maybeMatrnr = re.match(pattern, line, re.IGNORECASE)
            if maybeMatrnr is not None:
                matrnr = maybeMatrnr.group(1)

if not name or name == '???' or not matrnr or matrnr == '???':
    sys.exit('Please add your name and matriculation number to \'student_info.txt\'.\n'
             'Try adding:\n'
             '   Name: \n'
             '   MatrNr: ')

print('Assigning this submission to:\n'
      f'   {name} with MatrNr. {matrnr}\n'
      'Please double-check that this is correct!')
