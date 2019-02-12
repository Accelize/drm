Installation
============

This section explain how to install and build Accelize DRM library.

Supported OS
------------

Software requirements:

* The Accelize DRM library requires a compiler with full C++11 support.
* The Accelize DRM Python library requires Python >= 3.5.

Following OS are tested and supported by Accelize:

* Centos 7 [#f1]_
* Debian 9 Stretch
* Ubuntu 16.04 LTS Xenial
* Ubuntu 18.04 LTS Bionic

.. [#f1] With Python 3.6 from EPEL repository for Accelize DRM Python library.

Following OS are tested by not officially supported by Accelize:

* Debian Testing
* Fedora (Last stable version)

Installation from packages
--------------------------

Install the Accelize repository
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Packages are hosted on the Accelize repository.

.. todo:: Package repository configuration

.. note:: Packages are signed for security.

C/C++ Library
~~~~~~~~~~~~~

The C/C++ library are required to run application that depends on Accelize DRM.

It can be installed using package manager:

.. code-block:: bash
    :caption: On Debian, Ubuntu

    sudo apt install -y libaccelize-drm

.. code-block:: bash
    :caption: On RHEL, CentOS

    sudo yum install -y libaccelize-drm

Python Library
~~~~~~~~~~~~~~

The Python library allow to use Accelize DRM with Python 3.
It also bundle Cython headers to the C/C++ Accelize DRM library.

It can be installed using package manager:

.. code-block:: bash
    :caption: On Debian, Ubuntu

    sudo apt install -y python3-accelize-drm

.. code-block:: bash
    :caption: On RHEL, CentOS

    sudo yum install -y python3-accelize-drm

.. note:: The ``libaccelize-drm`` package is automatically installed with the
          ``python3-accelize-drm`` package.

The Python library can also be installed with Pip. In this case, the library
will be compiled from source distribution and require the Accelize DRM C/C++
development package.

.. code-block:: bash

    pip3 install -U accelize_drm

C/C++ Development package
~~~~~~~~~~~~~~~~~~~~~~~~~

The development package contain C/C++ header files and documentation.
It is required to build application that depends on Accelize DRM.

It can be installed using package manager:

.. code-block:: bash
    :caption: On Debian, Ubuntu

    sudo apt install -y libaccelize-drm-dev

.. code-block:: bash
    :caption: On RHEL, CentOS

    sudo yum install -y libaccelize-drm-devel

.. note:: The ``libaccelize-drm`` is automatically installed with the
          Development package.

.. note:: The development package also install an offline version of this
          documentation that can be found read by opening
          ``/usr/share/accelize/drm/doc/html/index.html``.

Compilation from sources
------------------------

Requirements
~~~~~~~~~~~~

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
    :caption: On Debian, Ubuntu

    sudo apt install -y git make g++ libcurl4-openssl-dev libjsoncpp-dev

.. code-block:: bash
    :caption: On RHEL, CentOS, Fedora

    sudo yum install -y git make gcc gcc-c++ libcurl-devel jsoncpp-devel

We recommend to install CMake as Python package to get a recent version
(Some version packaged on some OS are too old to build the Accelize DRM library)

You need to first install Python3, Pip and then CMake.

.. code-block:: bash
    :caption: On Debian, Ubuntu

    sudo apt install -y python3 python3-dev python3-pip
    pip3 install -U cmake

.. code-block:: bash
    :caption: On Centos7

    # Install EPEL repository to get a recent Python version
    sudo yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm

    # Install Python3.6
    sudo yum install -y python36 python36-devel

    # Install Pip
    sudo python36 -m ensurepip
    sudo ln -s /usr/local/bin/pip3 /usr/bin/pip3

    # Install Cmake
    pip3 install cmake

To build Python Library
^^^^^^^^^^^^^^^^^^^^^^^

Python 3.5 or more

Python packages:

* setuptools
* wheel
* cython >= 0.28

Run following command to install requirements:

.. code-block:: bash

    pip3 install -U setuptools wheel cython

To build documentation
^^^^^^^^^^^^^^^^^^^^^^

Utilities (Always required):

* doxygen

Python packages (Required for full documentation):

* sphinx
* breathe
* sphinx_rtd_theme

Run following command to install requirements:

.. code-block:: bash
    :caption: On Debian, Ubuntu

    sudo apt install -y doxygen
    pip3 install -U sphinx breathe sphinx_rtd_theme

.. code-block:: bash
    :caption: On RHEL, CentOS, Fedora

    sudo yum install -y doxygen
    pip3 install -U sphinx breathe sphinx_rtd_theme

To build packages
^^^^^^^^^^^^^^^^^

RPM package (For RHEL, CentOS, Fedora)
``````````````````````````````````````

* rpm-build

Run following command to install requirements:

.. code-block:: bash

    sudo yum install -y pkg-config

DEB Packages (For Debian, Ubuntu)
`````````````````````````````````

* pkg-config
* dpkg-dev

Run following command to install requirements:

.. code-block:: bash

    sudo apt install -y pkg-config dpkg-dev

To run tests
^^^^^^^^^^^^

Python packages:

* pytest

Run following command to install requirements:

.. code-block:: bash

    pip3 install -U pytest

Debug tests
^^^^^^^^^^^

.. note:: Required to run tests in Debug build mode only.

Utilities:

* abi-compliance-checker

Run following command to install requirements:

.. code-block:: bash
    :caption: On Debian 9 or more , Ubuntu 18.04 or more

    sudo apt install -y abi-compliance-checker abi-dumper

.. code-block:: bash
    :caption: On Debian < 9 , Ubuntu < 18.04

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
    :caption: On RHEL, CentOS, Fedora

    sudo yum install -y abi-compliance-checker

Full test scenario support
``````````````````````````

.. note:: Required to run the full testing scenario only

Python packages:

* tox

Run following command to install requirements:

.. code-block:: bash

    pip3 install -U tox

Coverage support
````````````````

.. note:: Required to run tests in Debug build mode with coverage only.

Utilities:

* lcov

Python packages:

* pytest-cov
* cython

Run following command to install requirements:

.. code-block:: bash
    :caption: On Debian, Ubuntu

    sudo apt install -y lcov
    pip3 install -U pytest-cov cython

.. code-block:: bash
    :caption: On RHEL, CentOS, Fedora

    sudo yum install -y lcov
    pip3 install -U pytest-cov cython

Building
~~~~~~~~

Clone Accelize DRM library repository:

.. code-block:: bash

    git clone https://github.com/Accelize/drmlib.git --depth 1

Create a build directory and move to it (Example with a build directory
relative to sources directory):

.. code-block:: bash

    mkdir build
    cd build

Run CMake and compile.

It is possible to specify following options to CMake to build optional
components:

* ``-DPYTHON3=ON``: Build Python library.
* ``-DDOC=ON``: Build documentation. The Python library is required to get the
  full library documentation in *Sphinx* HTML format, else only the
  documentation for the C/C++ API is generated in *Doxygen* HTML format.
* ``-DTESTS=ON``: Generates testings related files.
* ``-DCOVERAGE=ON``: If ``-DTESTS`` is ``ON``, compile with coverage support.

.. note:: Build the development package require both ``-DPYTHON3=ON`` and
          ``-DDOC=ON`` options.

.. code-block:: bash

    cmake -DPYTHON3=ON -DDOC=ON ..
    make -j

Optionally, it is possible install libraries system wide:

.. code-block:: bash

    make install

Optionally, it is possible to build packages:

.. code-block:: bash

    make package

After built, it is possible to found following generated components in build
directory:

* C++ library as files starting by ``libaccelize_drm.``
* C library as files starting by ``libaccelize_drmc.``
* C/C++ headers in ``include`` directory.
* Python library in ``python3_bdist`` directory.
* Python library sources in ``python3_src`` directory.
* Packages in ``packages`` directory.
* Documentation in HTML format in ``doc_html`` directory.

Running tests
~~~~~~~~~~~~~

This chapter explain how to run Accelize DRM library tests.

.. warning:: Tests requires a real FPGA board and supported test driver
             (See ``--fpga_driver`` option below).

Tests support following options:

* **--backend**: Select library API to use as backend
  (Supported from pytest only). Possibles values ``c`` or ``c++``.
  Default: ``c++``.

* **--fpga_driver**: Select FPGA driver to use. Default: ``aws_f1``.
  Possibles values:

  * *aws_f1*: Amazon Web Service FPGA instances (f1.2xlarge, f1.4xlarge).

* **--fpga_slot_id**: Set FPGA slot. Default: ``0``.

* **--drm_controller_base_address**: Set DRM Controller IP base address.
  Default: ``0``.

* **--cred**: Specify the path to a ``cred.json`` file containing valid
  Accelize credentials to use as base to run tests. Default: ``./cred.json``.

* **--server**: Specify metering server URL.
  Default: ``https://master.metering.accelize.com``

* **--library_verbosity**: Specify Accelize DRM library verbosity.
  Possibles values: ``0`` to ``5``. Default: ``4``.

Running test on previously build environment
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. note:: Prior to run tests, CMake need to be called with ``-DTESTS=ON`` &
          ``-DPYTHON3=ON`` options.

Tests run with pytest, it support previously defined options.

.. code-block:: bash

    # It is required to set LD_LIBRARY_PATH to the "build" directory to
    # Allow import of the library directly in the build environment.
    export LD_LIBRARY_PATH=path_to_build_directory

    # Run test tests with pytest
    pytest --cred=~/my_application/cred.json

Coverage
````````

Coverage support can be enabled by adding ``-DCOVERAGE=ON`` &
``-DCMAKE_BUILD_TYPE=Debug`` options.

Then, run pytest with pytest-cov options to have Python library coverage:

.. code-block:: bash

    export LD_LIBRARY_PATH=path_to_build_directory

    pytest --cred=~/my_application/cred.json --cov=accelize_drm"

The C/C++ library coverage is generated using gcov and can be retrieved after
tests using lcov:

.. code-block:: bash

    lcov --capture --directory . --output-file coverage.info -q >/dev/null 2>&1
    lcov -r coverage.info '/usr/include/*' '*/drm_controller_sdk/*' -o coverage.info -q
    lcov --list coverage.info
    genhtml coverage.info -q --legend -o coverage

This output an HTML report in the ``coverage`` directory.

Running test on installed library
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This allow to test library installed with package or with ``make install``.

From the DRM library repository library, simply run the pytest command:

.. code-block:: bash

    pytest --cred=~/my_application/cred.json

Running full tests scenario
^^^^^^^^^^^^^^^^^^^^^^^^^^^

There is a full testing scenario that exists.

This scenario performs following actions:

* Build the library in ``debug``, ``release`` and/or ``install`` mode.
* Run Both C and C++ backend tests.
* Install libraries (``install`` mode only).
* Generated package and documentation (``install`` mode only).
* Combine all tests coverage and generate Python/C/C++ coverage report
  (``debug`` mode only).

Tox tests are executed directly from the DRM library repository directory
(Directory where the ``tox.ini`` file is).

Tests run with tox, it support previously defined options passed after ``--``.
The ``--backend`` option si not supported because managed by tox.

.. code-block:: bash
    :caption: Running the full scenario

    tox -- --cred=~/my_application/cred.json

.. warning:: Running Tox with `sudo` may be required to run `build-install`
             scenario and accessing FPGA in `c` and `cpp` scenarios.

It is possible to reduce the scenario scope with the ``-e`` tox argument:

.. code-block:: bash
    :caption: Running Debug scenario only (with coverage)

    tox -e build-debug,cpp-debug,c-debug,coverage-debug -- --cred=~/my_application/cred.json

Coverage reports can be found in the ``report`` directory in the tox debug build
environment (By default: ``./.ini/debug/build/report``)

.. code-block:: bash
    :caption: Running Release scenario only

    tox -e build-release,cpp-release,c-release -- --cred=~/my_application/cred.json

.. code-block:: bash
    :caption: Running Install scenario only

    sudo tox -e build-install,cpp-install,c-install -- --cred=~/my_application/cred.json

Tox can performs some tests in parallel with the ``-p all`` option:

.. warning:: Running parallel tests requires 2 FPGA with slots ``0`` and ``1``.

.. note:: In this case, the ``--fpga_slot_id`` is not supported because managed
          by tox.

.. code-block:: bash
    :caption: Running full scenario in parallel

    tox -p all -- --cred=~/my_application/cred.json
