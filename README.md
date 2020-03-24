![Screenshot](images/AccelDRM_lock.png)

![Build Status](https://codebuild.eu-west-1.amazonaws.com/badges?uuid=eyJlbmNyeXB0ZWREYXRhIjoiVXRTOGxGaW5hZ1BzVGtoa0pBODhPanFlL3RpbFJPeGo5dHNxN3Uwb1ZuUmZOc1VkejBOeGk2WGVodzZPK2NvdS9WWVZUSlJhZXFMbEgrejN1VDN6TE13PSIsIml2UGFyYW1ldGVyU3BlYyI6IkhtMFRSeW1aRzVWVUpZcVQiLCJtYXRlcmlhbFNldFNlcmlhbCI6MX0%3D&branch=master)
[![codecov](https://codecov.io/gh/Accelize/drmlib/branch/master/graph/badge.svg)](https://codecov.io/gh/Accelize/drmlib)

# :closed_lock_with_key: Accelize DRM library : AccelDRM

The Accelize DRM solution protects FPGA applications.

The Accelize DRM library operates the DRM from the software side of the
application.

This library is responsible for activating the DRM, communicating with the
Accelize web service and managing metering sessions.

This library provides **API** for :
* C
* C++
* Python

Interested in AccelDRM ? Checkout our [Documentation](http://accelize.s3-website-eu-west-1.amazonaws.com/documentation/stable)!

# :key: Accelize licensing solution

The Accelize licensing technology is offered in two distinct modes:

* Static licensing :
A file-based scheme implemented by statically packaging the license key into an
encrypted license file, stored locally on the server that hosts the FPGA card.

* Dynamic licensing :
A server-based scheme implemented by delivering license keys from a license server.
Specifically, the license server delivers a regular stream of time-limited single-use license keys.

Take a look at our [documentation](http://accelize.s3-website-eu-west-1.amazonaws.com/documentation/stable/#licensing-modes) for more information !

# :wrench: Accelize Architecture Overview

The Accelize DRM solution comprises 3 layers:

* FPGA HDL IPs that must be embedded into the FPGA design. Specifically, exactly one DRM Controller IP,
and one or more DRM Activator IPs. The Activators must be embedded within each protected function.
These IPs are delivered in the DRM HDK.
* Host The DRM client, a lightweight service that executes on the host CPU, whose main function is to connect
the FPGA DRM IPs with either the DRM Web Service (dynamic licensing) or a local license key file (static licensing).
The DRM client is delivered as a DRM Library in C/C++ and Python.
* Web Service A fully managed DRM Web Service operated by Accelize. The Web service is only used in dynamic
licensing and handles user authentication, licensing and metering. Upon special request,
the DRM Web Service can deployed on-premise.

More information in our [documentation](http://accelize.s3-website-eu-west-1.amazonaws.com/documentation/stable/#licensing-modes) !

## :computer: Accelize Distribution Platform

For a quick tour about the Accelize Platform integration steps, please watch [this video](https://youtu.be/7cb_ksLTcRk)

## :heavy_exclamation_mark: Prerequisites

To access the Accelize Web Service you need an Internet connection which allows
outbound HTTPS connection to Accelize [server](https://master.metering.accelize.com)

## :registered: License

Please consult [license](LICENSE)

## :floppy_disk: Changelog

Please consult [CHANGELOG](CHANGELOG)
