# ov-client

![Ubuntu make](https://github.com/gisogrimm/ov-client/workflows/Ubuntu%20make/badge.svg?branch=master)
![MacOS make](https://github.com/gisogrimm/ov-client/workflows/MacOS%20make/badge.svg?branch=master)

## The OVBOX system

The OVBOX system is a system for low-delay network audio communication. It features individual 3D virtual acoustics, integration of head tracking for dynamic binaural rendering as well as rendering of distributed moving sound sources.

The system consists of several components: the *client device*, which is this software (optionally running on a dedicated computer, see installation instructions), a *configuration server* with a web interface for the users and a REST API, and a list of *room servers*, which handle the session.

The source code of the configuration server can be found [here](https://github.com/gisogrimm/ov-webfrontend). The source code of the room server can be found [here](https://github.com/gisogrimm/ov-server).

## Installation instructions

For installation instructions see [Wiki pages](https://github.com/gisogrimm/ovbox/wiki/Installation).


## User manual

A user manual for the ORLANDOviols configuration server can be found
on the [wiki](https://github.com/gisogrimm/ovbox/wiki).

## ovbox

ov-client is a re-implementation of the original ovbox. It can run on
Raspberry Pi (models 4B and 3B+), Ubuntu Linux and on MacOS. See installation instructions for details.
