#! /usr/bin/env python3
# coding=utf-8
"""
Prepare the petalinux recipe used to generate the package of the DRM Library package for SoM targets
"""

from argparse import ArgumentParser
from os import pardir, makedirs, getcwd
from os.path import abspath, realpath, dirname, join, isdir
from subprocess import run, PIPE, CalledProcessError
from string import Template
import shlex
import hashlib
import re


SCRIPT_DIR = dirname(realpath(__file__))

run_args = dict(stdout=PIPE, stderr=PIPE, universal_newlines=True)

def run_git(git_cmd, from_dir=None):
    if from_dir:
        git_cmd = git_cmd.replace('git', f'git -C {from_dir}')
    result = run(shlex.split(git_cmd), **run_args)
    result.check_returncode()
    return result.stdout.strip()


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
    
    git_options = ''
    
    # Check git version if -C option is given
    from_dir = None
    if abspath(SCRIPT_DIR) != abspath(getcwd()):
        from_dir = abspath(SCRIPT_DIR)
    
    # Get Version in CMakeLists.txt
    cmake_file = join(SCRIPT_DIR, pardir, 'CMakeLists.txt')
    with open(cmake_file, 'rt') as f:
        cmake_str = f.read()
    cmake_version_str = re.search(r'ACCELIZEDRM_VERSION\s+([^)]+)', cmake_str).group(1)
    print('CMakeFiles version =', cmake_version_str)
    m = re.match(r'(\d+\.\d+\.\d+)(-.*)?', cmake_version_str)
    cmake_ver = m.group(1)
    cmake_rev = m.group(2)
    if cmake_rev.startswith('-'):
        cmake_rev = cmake_rev[1:].replace('.','')
    
    # Compute LICENSE MD5
    license_file = join(SCRIPT_DIR, pardir, 'LICENSE')
    with open(license_file, 'rt') as f:
        license_str = f.read()
    md5 = hashlib.md5(license_str.encode()).hexdigest()
    print('LICENSE MD5 =', md5)

    # Get current branch
    git_branch = run_git('git rev-parse --abbrev-ref HEAD', from_dir=from_dir)
    git_options += f';branch={git_branch}'
    print('GIT branch =', git_branch)

    # Get current commit
    git_commit = run_git('git rev-parse HEAD', from_dir=from_dir)
    git_commit_short = git_commit[:7]
    print('GIT commit =', git_commit)
    
    # Check if working tree is dirty
    try:
        run_git('git diff-index --quiet HEAD', from_dir=from_dir)
        dirty = False
    except CalledProcessError:
        dirty = True
    print('dirty=', dirty)
    
    # Get current tag if any
    git_tag = run_git('git tag --points-a HEAD', from_dir=from_dir)
    if git_tag:
        print('GIT tag =', git_tag)
        assert cmake_version_str == git_tag, 'Tag must match version in CMakeLists.txt'
        git_options += f';tag={git_tag}'        
    
    # Create PV variable
    pv = cmake_ver
    if not git_tag:
        pv += f'+{git_commit_short}'
    if dirty:
        pv += '-dirty'
    
    # Create PR variable    
    pl_version = args.petalinux_version.replace('.', '_')
    if cmake_rev:
        pr = f'{cmake_rev}.pl{pl_version}'
    else:
        pr = f'1.pl{pl_version}'
        
    print(f'PV: {pv}')
    print(f'PR: {pr}')
    
    # Populate template file
    with open(join(SCRIPT_DIR, 'libaccelize-drm', 'tmpl.libaccelize-drm.bb')) as f:
        tmpl_str = f.read()
    tmpl_obj = Template(tmpl_str)    
    dst_str = tmpl_obj.substitute(TMPL_LICENSE_MD5=md5,
        TMPL_GIT_OPTIONS=git_options, TMPL_GIT_COMMIT=git_commit, 
        TMPL_PV=pv, TMPL_PR=pr
    )
    
    # Save generated file
    if not isdir(args.output):
        makedirs(args.output)
    with open(join(args.output, 'libaccelize-drm.bb'), 'wt') as f:
        f.write(dst_str)
    
    print('Done')
