#! /usr/bin/env python3
# coding=utf-8
"""
Prepare the petalinux recipe used to generate the package of the DRM Library package for SoM targets
"""

from argparse import ArgumentParser
from os import pardir, makedirs
from os.path import realpath, dirname, join, isdir
from subprocess import run, PIPE
from string import Template
import hashlib
import re


SCRIPT_DIR = dirname(realpath(__file__))


if __name__ == '__main__':    
    parser = ArgumentParser(
        description=
        'Prepare the petalinux recipe to generate the DRM Library package for SoM target. '
        'This tool replaces the right figures into the DRM Library recipe so that '
        'the generated package has the correct version numbers. '
        'This script is called by the makefile used to generate the package'
        )
    parser.add_argument(
        'petalinux_version', help=
        'Version of petalinux to use.'
        'Expected format: 2020_2_2, 2021_1'        
    )
    parser.add_argument('--output', '-o', default='output',
        help='Specify output directory.')
    args = parser.parse_args()
    
    run_args = dict(stdout=PIPE, stderr=PIPE, universal_newlines=True)
    git_options = ''
    
    # Get Version in CMakeLists.txt
    cmake_file = join(SCRIPT_DIR, pardir, 'CMakeLists.txt')
    with open(cmake_file, 'rt') as f:
        cmake_str = f.read()
    cmake_version = re.search(r'ACCELIZEDRM_VERSION\s+([^)]+)', cmake_str).group(1)
    print('CMakeFiles version =', cmake_version)
    
    # Compute LICENSE MD5
    license_file = join(SCRIPT_DIR, pardir, 'LICENSE')
    with open(license_file, 'rt') as f:
        license_str = f.read()
    md5 = hashlib.md5(license_str.encode()).hexdigest()
    print('LICENSE MD5 =', md5)
    
    # Get current branch
    result = run(['git', 'rev-parse', '--abbrev-ref', 'HEAD'], **run_args)
    result.check_returncode()
    git_branch = result.stdout.strip()
    git_options += f';branch={git_branch}'
    print('GIT branch =', git_branch)

    # Get current tag if any
    result = run(['git', 'tag', '--points-at', 'HEAD'], **run_args)
    result.check_returncode()
    git_tag = result.stdout.strip()
    if git_tag:
        assert cmake_version == git_tag
        git_options += f';tag={git_tag}'
        pv = git_tag
        print('GIT tag =', git_tag)
    else:
        pv = '1.0.0'
        
    # Get current commit
    result = run(['git', 'rev-parse', 'HEAD'], **run_args)
    result.check_returncode()
    git_commit = result.stdout.strip()
    print('GIT commit =', git_commit)

    # Populate template file
    with open(join(SCRIPT_DIR, 'libaccelize-drm', 'tmpl.libaccelize-drm.bb')) as f:
        tmpl_str = f.read()
    tmpl_obj = Template(tmpl_str)    
    dst_str = tmpl_obj.substitute(TMPL_LICENSE_MD5=md5,
        TMPL_GIT_OPTIONS=git_options, TMPL_GIT_COMMIT=git_commit, 
        TMPL_PV=pv, TMPL_PL_VERSION=args.petalinux_version
    )
    
    # Save generated file
    if not isdir(args.output):
        makedirs(args.output)
    with open(join(args.output, 'libaccelize-drm.bb'), 'wt') as f:
        f.write(dst_str)
    
    print('Done')
    
    


