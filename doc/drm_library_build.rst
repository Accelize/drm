DRM library build & test
========================

This section explain in detail how to build and test the library. This
section is only useful for DRM library contributors, if you simply want to
install the library, see :doc:`drm_library_installation`.

CMake options are used to select which components are generated amongst the following ones:

* Documentation
* Python wrapper
* Testing materials
* Reporting materials

For more details about available CMake options, refer to `Build CMake configuration`_.


Requirements
````````````

Minimal requirements
::::::::::::::::::::

Utilities:
 * git
 * cmake >= 3.12
 * make (Or any CMake supported build tool)
 * GCC, G++ >= 4.8 (Or any compatible C++11 compiler)

Libraries:
 * libcurl-devel
 * jsoncpp-devel

Run following commands to install requirements:

.. code-block:: bash
    :caption: On Debian 10 and more, Ubuntu 18.10 and more

    # Ensure Apt cache is up to date
    sudo apt update

    # Install packages
    sudo apt install -y git make g++ libcurl4-openssl-dev libjsoncpp-dev cmake

.. code-block:: bash
    :caption: On Debian < 10, Ubuntu < 18.10

    # Ensure Apt cache is up to date
    sudo apt update

    # Install packages
    sudo apt install -y git make g++ libcurl4-openssl-dev libjsoncpp-dev

    # Ensure Pip3 is installed
    sudo apt install -y python3-pip

    # Install a recent version of Cmake using pip
    sudo pip3 install -U cmake

.. code-block:: bash
    :caption: On RHEL 8, CentOS 8

    # Ensure EPEL repository is installed
    sudo dnf install -y epel-release

    # Install packages
    sudo dnf install -y git make gcc gcc-c++ libcurl-devel jsoncpp-devel cmake

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    # Ensure EPEL repository is installed
    sudo yum install -y epel-release

    # Install packages
    sudo yum install -y git make gcc gcc-c++ libcurl-devel jsoncpp-devel

    # Ensure Pip3 is installed
    sudo yum install -y python36-pip

    # Install a recent version of Cmake using pip
    sudo pip3 install -U --prefix /usr cmake

.. code-block:: bash
    :caption: On Fedora

    sudo dnf install -y git make gcc gcc-c++ libcurl-devel jsoncpp-devel cmake

Python 3 library option
:::::::::::::::::::::::

This step is required only if you want to:

* Generate the Python 3 library (using ``-DPYTHON3=ON`` option with CMake)
* Generate the Sphinx documentation (using ``-DDOC=ON`` option with CMake)
* Complete the `Run tests`_ section.

Otherwise you can jump to the next step.

Packages:
 * Python3-devel

Python packages:
 * setuptools
 * wheel
 * cython >= 0.28

Run following command to install requirements:

.. code-block:: bash
    :caption: On Debian, Ubuntu

    sudo apt install -y python3 python3-dev python3-pip
    sudo pip3 install -U setuptools wheel cython

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    # Install EPEL repository to get a recent Python version
    sudo yum install -y epel-release

    # Install Python3.6
    sudo yum install -y python36-pip python36-devel

    # Install Python Packages
    sudo pip3 install -U --prefix /usr  setuptools wheel cython

.. code-block:: bash
    :caption: On Fedora, RHEL 8, CentOS 8

    sudo dnf install -y python3-devel python3-pip
    sudo pip3 install -U setuptools wheel cython

Documentation generation option
:::::::::::::::::::::::::::::::

This step is required only if you want to:

* Generate the C/C++ documentation (using ``-DDOC=ON`` option with CMake)
* Generate the full Sphinx documentation (using ``-DDOC=ON -DPYTHON3=ON`` options with CMake)

Otherwise you can jump to the next step.

Utilities:
 * doxygen

Python packages (Required for full documentation):
 * sphinx
 * breathe
 * sphinx_rtd_theme

Run following command to install requirements:

.. code-block:: bash
    :caption: On Debian, Ubuntu

    sudo apt install -y doxygen
    sudo pip3 install -U sphinx breathe sphinx_rtd_theme

.. code-block:: bash
    :caption: On RHEL 8, CentOS 8

    sudo dnf install -y --enablerepo=PowerTools doxygen
    sudo pip3 install -U sphinx breathe sphinx_rtd_theme

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    sudo yum install -y doxygen
    sudo pip3 install -U --prefix /usr sphinx breathe sphinx_rtd_theme

.. code-block:: bash
    :caption: On Fedora

    sudo dnf install -y doxygen
    sudo pip3 install -U sphinx breathe sphinx_rtd_theme

Test generation option
::::::::::::::::::::::

.. warning:: This dependency is mandatory to complete the `Run tests`_ section.

This step is required only if you want to:

* Run the test suite (using -DTESTS=ON` option with CMake)

Otherwise you can jump to the next step.

Python packages:
 * pytest

Run following command to install requirements:

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    sudo pip3 install -U --prefix /usr pytest

.. code-block:: bash
    :caption: On others

    sudo pip3 install -U pytest

Package generation option
:::::::::::::::::::::::::

This step is required only if you want to:

* Generate the installation packages (using -DPKG=ON` option with CMake)

Otherwise you can jump to the next step.

Before going further, make sure the section `Python 3 option`_ has been completed.

RPM package (For RHEL, CentOS, Fedora)
''''''''''''''''''''''''''''''''''''''

Required to build packages:
 * rpm-build

Required to sign packages:
 * rpm-sign
 * gnupg

To install the required utilities, run the following command:

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    sudo yum install -y rpm-build rpm-sign gnupg

.. code-block:: bash
    :caption: On Fedora, RHEL 8, CentOS 8

    sudo dnf install -y rpm-build rpm-sign gnupg

DEB Packages (For Debian, Ubuntu)
'''''''''''''''''''''''''''''''''

Required to build package:
 * pkg-config
 * dpkg-dev
 * file

Required to sign packages:
 * dpkg-sig
 * gnupg

To install required utilities, run the following command:

.. code-block:: bash

    sudo apt install -y pkg-config dpkg-dev dpkg-sig gnupg file

ABI check option
::::::::::::::::

This step is required only if you want to:

* Run the test suite in Debug mode (using -DCMAKE_BUILD_TYPE=Debug` option with CMake)

Otherwise you can jump to the next step.

Utilities:
 * abi-compliance-checker

To install requirements run the following command:

.. code-block:: bash
    :caption: On Debian 9 or more, Ubuntu 18.04 or more

    sudo apt install -y abi-compliance-checker abi-dumper

.. code-block:: bash
    :caption: On Debian < 9, Ubuntu < 18.04

    # ABI compliance checker is not available as package for theses version and
    # needs to be installed manually.
    sudo apt install -y libelf-dev elfutils dh-autoreconf exuberant-ctags
    git clone https://github.com/lvc/abi-compliance-checker --depth 1
    cd abi-compliance-checker
    make -j
    make install
    cd ..
    git clone https://github.com/lvc/abi-dumper --depth 1
    cd abi-dumper
    make -j
    make install
    cd ..

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    sudo yum install -y abi-compliance-checker

.. code-block:: bash
    :caption: On Fedora, RHEL 8, CentOS 8

    sudo dnf install -y abi-compliance-checker

Coverage option
:::::::::::::::

This step is required only if you want to:

* Compile the C/C++ with the -coverage option (using ``-DCOVERAGE=ON`` option with CMake)
* Generate the coverage report after the test suite execution in Debug (using
  ``-DCMAKE_BUILD_TYPE=Debug -DTEST=ON -DCOVERAGE=ON`` options with CMake)

Otherwise you can jump to the next step.

Utilities:
 * lcov

Python packages:
 * pytest-cov

Run following command to install requirements:

.. code-block:: bash
    :caption: On Debian, Ubuntu

    sudo apt install -y lcov
    sudo pip3 install -U pytest-cov

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    sudo yum install -y lcov
    sudo pip3 install -U --prefix /usr pytest-cov

.. code-block:: bash
    :caption: On Fedora, RHEL 8, CentOS 8

    sudo dnf install -y lcov
    sudo pip3 install -U pytest-cov

Automation with tox
:::::::::::::::::::

This step is required only if you want to use tox to automate some execution scenarios.
Otherwise you can jump to the next step.

Python packages:
 * tox

Run following command to install requirements:

.. code-block:: bash
    :caption: On RHEL 7, CentOS 7

    sudo pip3 install -U --prefix /usr tox

.. code-block:: bash
    :caption: On others

    sudo pip3 install -U tox


Build CMake configuration
`````````````````````````

1. Clone Accelize DRM library repository and move to it:

.. code-block:: bash

    git clone https://github.com/Accelize/drmlib.git --recursive --depth 1
    cd drmlib

2. Create a build directory and move to it:

.. code-block:: bash

    mkdir build
    cd build

3. Run CMake to build your configuration:

.. code-block:: bash

    cmake ..

Use the following options to build optional components:

* ``-DPYTHON3=ON``: Build Python library as a wrapper of the C/C++ library
* ``-DDOC=ON``: Build documentation. The Python library is required to get the
  full library documentation in *Sphinx* HTML format. Otherwise only the
  documentation for the C/C++ API is generated in *Doxygen* HTML format.
* ``-DTESTS=ON``: Generate the testing materials and run test suite.
* ``-DCOVERAGE=ON``: If ``-DTESTS=ON``, compile with coverage support.
* ``-DPKG=ON``: Generate the installation packages.
* ``-DCMAKE_BUILD_TYPE=Debug``: Compile in Debug mode.
* ``-DAWS=ON``: Run full test suite when executed on AWS f1 instance.

.. note:: Build the development package require both ``-DPYTHON3=ON`` and
          ``-DDOC=ON`` options.

.. code-block:: bash
   :caption: Build Python Library and Sphinx-like documentation

   cmake -DPYTHON3=ON -DDOC=ON ..

Compile CMake configuration
```````````````````````````

Once the CMake configuration built, you can either:

* Compile:

  .. code-block:: bash

    make -j

  From here you can test your compiled library in section `Run tests`_.

* or you can directly install the library on your system:

  .. code-block:: bash

    sudo make install

  From here you can test your compiled library in section `Run tests`_.

* or you can build the installation packages (Require cmake ``-DPKG=ON`` option):

  .. code-block:: bash

    make package


Generated output
````````````````

Depending on your CMake configuration, the *build* directory will contain
the following components:

* C++ library named as ``libaccelize_drm``
* C library named as ``libaccelize_drmc``
* C/C++ headers located in ``include`` directory.
* Python library located in ``python3_bdist`` directory.
* Python library sources located in ``python3_src`` directory.
* Packages located in ``packages`` directory.
* Documentation in HTML format located in ``doc_html`` directory.

Run tests
`````````

This section explains how to run Accelize DRM python library tests.

.. warning:: Following tests require a real FPGA board and associated driver installed.
             Refer to `Supported OS`_ to get the list of tested OS.

.. important:: The tests described below are based on the Python DRM library
               and *pytest* module. So make sure sections `Python 3 option`_
               and `Test generation option`_ have been completed and that you have
               run the CMake command with the ``-DTESTS=ON -DPYTHON3=ON`` options.

Test command
::::::::::::

Usage
'''''

Here is the test command:

.. code-block:: bash

    # LD_LIBRARY_PATH must be set to the "build" directory so that
    # the library can be directly imported in the build environment.
    export LD_LIBRARY_PATH=path_to_build_directory

    # Run tests with pytest
    pytest --cred=path_to_cred.json [options]

.. warning:: Depending on your execution platform environment and the driver requirements you
             might need to execute the comment with `sudo`:

             .. code-block:: bash

                sudo LD_LIBRARY_PATH=path_to_build_directory pytest --cred=path_to_cred.json [options]

``path_to_cred.json`` is the path to your credentials file.
For more details refer to the section 'Credentials file' in :doc:`drm_configuration`

Command options are:

--backend=<c++|c>           Select library API to use as backend
                            (Supported from pytest only). Default: ``c++``.

--fpga_driver=key_name      Select FPGA driver to use. Default: ``aws_f1``.
                            Possibles key_name values:
                            * **aws_f1**: Amazon Web Service FPGA instances (f1.2xlarge, f1.4xlarge).

--fpga_slot_id=integer      Set FPGA slot. Default: ``0``.

--drm_controller_base_address=address
                            Set DRM Controller IP base address.
                            Default: ``0``.

--cred=json_path            Specify the path to a ``cred.json`` file containing valid
                            Accelize credentials to use as base to run tests.
                            Default: ``./cred.json``.

--server=url                Specify metering server URL.
                            Default: ``https://master.metering.accelize.com``

--library_verbosity=<0..4>  Specify Accelize DRM library verbosity.
                            Possibles values: ``0`` to ``5``. Default: ``4``.

--library_log_format=<0|1>  Specify library log format: 0=short format, 1=long format.

--fpga_image=image          Select FPGA image to use for program the FPGA. By default,
                            use default FPGA image for the selected driver and last HDK version.
                            Set to empty string to not program the FPGA.

--hdk_version               Select FPGA image based on Accelize DRM HDK version.
                            By default, use default FPGA image for the selected driver
                            and last HDK version.

--integration               Run integration tests, needs 2 FPGA.

--endurance                 Run endurance tests, might require 2 FPGA.

--cov=accelize_drm          Run test with pytest-cov options to enable Python library coverage


Coverage
''''''''

.. important:: To enable coverage reporting the section `Coverage option`_ must have been performed.

Use --cov=accelize_drm option to enable the coverage with the test execution:

.. code-block:: bash

    sudo LD_LIBRARY_PATH=path_to_build_directory pytest --cred=~/my_application/cred.json --cov=accelize_drm

The C/C++ library coverage is generated using gcov and can be retrieved and gathered with python
coverage using lcov as follows:

.. code-block:: bash

    lcov --capture --directory . --output-file coverage.info -q >/dev/null 2>&1
    lcov -r coverage.info '/usr/include/*' '*/drm_controller_sdk/*' -o coverage.info -q
    lcov --list coverage.info
    genhtml coverage.info -q --legend -o coverage

The result is an HTML report located in the ``coverage`` directory.

.. note:: To enable coverage the CMake configuration must contain the
         ``-DCOVERAGE=ON -DCMAKE_BUILD_TYPE=Debug`` options.

.. note:: This procedure is fully and automatically managed using tox.
          See `Run tests partially`_ for more details.


Run full tests
::::::::::::::

This scenario performs following actions:

* Build the library in ``debug``, ``release`` and/or ``install`` mode.
* Get library for packages (``install`` mode only).
* Run Both C and C++ backend tests.
* Run Integration tests.
* Install libraries (``install`` mode only).
* Generate documentation (except in ``debug`` mode).
* Generate and export packages (``release`` mode only).
* Combine all tests coverage and generate Python/C/C++ coverage report
  (``debug`` mode only).

For simplicity and efficiency the tox utility is used to execute these scenarios.
Make sure the `Automation with tox`_ section has been performed.

Tox tests must be executed from the DRM library root directory where the ``tox.ini`` file is
located.
The usual test options can be used after the ``--`` delimiter.

.. code-block:: bash
    :caption: Running the full scenario

    tox -- --cred=~/my_application/cred.json [options]

.. note:: The ``--backend`` option is not supported because managed by tox.

.. warning:: Running Tox with `sudo` may be required to run `build-install`
             scenario and to access FPGA in `c` and `cpp` scenarios.

Run tests partially
:::::::::::::::::::

It is possible to reduce the scenario scope with the ``-e`` tox argument.
More information on `tox documentation`_.

.. _`tox documentation`: https://tox.readthedocs.io/en/latest/config.html


Some examples:

 * Build and run all tests with coverage in debug mode

   .. code-block:: bash

        tox -e build-debug,cpp-debug,c-debug,integration-debug,coverage-debug -- --cred=~/my_application/cred.json

   Coverage reports can be found in the ``report`` directory in the tox debug build
   environment (By default: ``./.tox/debug/build/report``)

* Build and run c and c++ tests in release mode

  .. code-block:: bash

        tox -e build-release,cpp-release,c-release -- --cred=~/my_application/cred.json

* Build and export packages

  .. code-block:: bash

        # Specify packages export directory
        export PACKAGES_DIR="~/packages"

        # Build and export
        tox -e build-release,export-release

* Build, install (using "make install") and run tests

  .. code-block:: bash

        sudo tox -e build-install,cpp-install,c-install -- --cred=~/my_application/cred.json

* Install from packages and run tests

  .. code-block:: bash

        # Get packages, by example build from a previous release scenario
        export PACKAGES_DIR="./.tox/release/build/packages"

        # Install packages and run tests
        sudo tox -e package-install,cpp-install,c-install -- --cred=~/my_application/cred.json


If your platform has 2 FPGA on 2 different PCIe slots, you can optimize the execution time
by parallelizing some tests with the ``-p all`` option:

.. note:: In this case, the ``--fpga_slot_id`` is not supported because managed
          by tox.

.. code-block:: bash
    :caption: Running full scenario in parallel

    tox -p all -- --cred=~/my_application/cred.json
