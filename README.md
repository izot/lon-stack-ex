LON Stack EX
============
*July 2022*

This document covers the following topics:

- Overview
- Supported Platforms
- Documentation
- Contributors

# Overview #

The EnOcean LON Stack EX enables developers to build networks of communicating devices for the Industrial Internet of Things. The LON protocol is an open standard defined by the ISO/IEC 14908 series of standards.

The EX stack enables Industrial IoT developers to build networks of communicating devices using any processor supporting the C or Python 3 programming language. Devices with the EX stack can exchange data with other LON devices using an easy-to-use publish-subscribe data model over IP or native LON channels. LON devices can collect data from physical sensors built to monitor the built environment including temperature, humidity, light-level, power-consumption, or moisture, and make the data available to other LON devices in the same network. Using data received from other LON devices or local sensors, LON devices can also control physical actuators such as LED dimmers, motor controllers, damper controllers, and solenoids. The LON protocol is an open standard defined by the ISO/IEC 14908 series of standards.

The EX stack implements optional features of the LON protocol that are suitable for edge servers and workstations implementing the LON protocol. The EX stack has been ported to ARM A-series prcoessors and to x64/x86 processors.

A smaller implementation of the LON stack is available in the LON Stack DX. The DX stack implements only the required  features of the LON protocol to provide a smaller footprint suitable for low-cost processors such as the ARM M-series processors.  The DX stack is available at http://github.com/izot/lon-stack-dx.

# Supported Platforms #

The EX stack is provided as source code that can be ported to a variety of 32-bit and 64-bit processors and operating systems.  The source code for the stacks is implemented in Python 3, C++, and C. The example applications are implemented in Python.  The Python code requires Python 3.2 or later.

# Documentation #

Documentation for the EX stack is available at <http://iecdocs.renesas.com>.

# Contributors #

- Rich Blomseth
- Glen Riley
- Fremont Baindbridge
- Gary Bartlett
- Kevin Blomseth
- Brian Fukano
- Bernd Gauweiler
- Marsha Pribadi
- Bob Walker
- Burcu Alaybeyi
- Luca Coppadoro
- Jon Kinsting
- Michael Milsner
- David McPartlan
- Christy Morrison
- Varun Nagaraj
- Mike Wytyshyn


