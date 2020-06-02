#! /usr/bin/env python3
# coding=utf-8
"""
Accelize DRM Python setup script

run "./setup.py --help-commands" for help.
"""
from os.path import dirname, abspath, join, isfile, splitext

# Set Package information
PACKAGE_INFO = dict(
    name='python_accelize_drm',
    version='@ACCELIZEDRM_VERSION@',
    description='@CPACK_COMPONENT_PYTHON3_DESCRIPTION@',
    long_description_content_type='text/markdown; charset=UTF-8',
    classifiers=[
        # Must be listed on: https://pypi.org/classifiers/
        'Development Status :: 5 - Production/Stable',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: Apache Software License',
        'Topic :: System :: Hardware :: Hardware Drivers',
        'Programming Language :: Cython',
        'Programming Language :: Python',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Operating System :: OS Independent'
    ],
    keywords='drm fpga',
    author='@CPACK_PACKAGE_VENDOR@',
    author_email='@ACCELIZE_EMAIL@',
    url='@CMAKE_PROJECT_HOMEPAGE_URL@',
    project_urls={
        'Documentation': '@PROJECT_DOCUMENTATION_URL@',
        'Accelize Website': '@ACCELIZE_HOMEPAGE@',
        'Contact': '@CPACK_PACKAGE_CONTACT@',
    },
    license='Apache License, Version 2.0',
    python_requires='>=3.5',
    setup_requires=['setuptools'],
    packages=['accelize_drm'],
    package_data={'accelize_drm': ['*.pxd']},
    zip_safe=False
)

SETUP_DIR = abspath(dirname(__file__))

# Get long description from readme
with open(join(SETUP_DIR, 'README.md')) as source_file:
    PACKAGE_INFO['long_description'] = source_file.read()

# Run setup
if __name__ == '__main__':
    from os import chdir, makedirs
    chdir(SETUP_DIR)

    # Check environment.
    if PACKAGE_INFO['version'].startswith('@'):
        raise RuntimeError(
            "Please, build using CMake instead of directly setup.py")

    # Sources files
    src_names = ('_accelize_drm.cpp', '_accelize_drmc.c')

    # Debug Configuration
    if '@CMAKE_BUILD_TYPE@'.lower() == 'debug':  # From CMAKE_BUILD_TYPE

        # Inplace build required for Coverage
        # Require call "build_ext" with "--inplace" from CMake
        from shutil import copy

        src_files = []
        for src in src_names:
            dst = join('accelize_drm', src)
            copy(splitext(join('src', src))[0] + '.pyx',
                 splitext(dst)[0] + '.pyx')
            src_files.append(dst)

        PACKAGE_INFO['package_data'] = {'accelize_drm': ['*']}

        # Debug compiler options
        compile_args = ["-g3", "-Og"]

        # Show source code in generated files
        cython_options = dict(emit_code_comments=True)

        # Generate annotation files
        get_report = True
        cythonize_kwargs = dict(annotate=True,

                                # Enable C/C++ debugging
                                gdb_debug=True,

                                # Allow coverage and profiling
                                compiler_directives={
                                    'linetrace': True,

                                    # Allow sphinx auto doc
                                    'embedsignature': True})

        # Allow coverage and profiling
        extension_kwargs = dict(define_macros=[
            ('CYTHON_TRACE', '1'), ('CYTHON_TRACE_NOGIL', '1')])

    # Release Configuration
    else:
        src_files = [join('src', src) for src in src_names]
        compile_args = ["-g0"]
        cython_options = dict(emit_code_comments=False)
        cythonize_kwargs = dict(compiler_directives={})
        extension_kwargs = {}
        get_report = False

        # Allow Sphinx to retrieve Cython modules docstrings
        if '@DOC@'.lower() != 'off':  # From CMake DOC Option
            cythonize_kwargs['compiler_directives']['embedsignature'] = True

    # Check if Cython if available
    try:
        import Cython  # noqa
        USE_CYTHON = True
    except ImportError:
        USE_CYTHON = False

        # Cython is mandatory if missing C/C++ precompiled sources
        for path in src_files:
            if not isfile(path):
                USE_CYTHON |= True

    # Update "setup_requires" with Cython
    if USE_CYTHON:
        PACKAGE_INFO['setup_requires'].append('cython>=0.28')

    # Configure build
    library_dirs = ['/usr/local/lib64', '/usr/local/lib']
    include_dir = 'accelize_drm'
    extension_kwargs.update(dict(
        library_dirs=library_dirs, runtime_library_dirs=library_dirs))
    ext_kwargs = [
        dict(name='accelize_drm._accelize_drm',
             extra_compile_args=compile_args + ['-std=c++11'],
             **extension_kwargs),
        dict(name='accelize_drm._accelize_drmc',
             extra_compile_args=compile_args, **extension_kwargs)]

    # Build Using Cython or C/C++ sources
    from setuptools import Extension, setup
    if USE_CYTHON:
        # Configure Cython options
        from Cython.Compiler import Options
        for key, value in cython_options.items():
            setattr(Options, key, value)

        # Compile Cython sources to C/C++ and generate extensions
        from Cython.Build import cythonize
        ext_modules = cythonize([Extension(
            sources=[splitext(src_files[i])[0] + '.pyx'], **ext_kwargs[i])
            for i in range(len(src_files))], include_path=[include_dir],
            **cythonize_kwargs)
    else:
        # Generate extensions from precompiled C/C++ sources
        ext_modules = [Extension(sources=[src_files[i]], **ext_kwargs[i])
                       for i in range(len(src_files))]

    # Run Setup
    setup(ext_modules=ext_modules, **PACKAGE_INFO)

    # Copy HTML report
    if get_report:
        report_dir = '../report/cython_annotate'
        makedirs(report_dir, exist_ok=True)
        for src in src_names:
            file_name = splitext(src)[0] + '.html'
            copy(join('accelize_drm', file_name),
                 join(report_dir, file_name))
