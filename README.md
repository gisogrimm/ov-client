# ov-client

[![Ubuntu 20.04 make](https://github.com/gisogrimm/ov-client/actions/workflows/ubuntu-latest.yml/badge.svg)](https://github.com/gisogrimm/ov-client/actions/workflows/ubuntu-latest.yml)
[![Ubuntu 22.04 make](https://github.com/gisogrimm/ov-client/actions/workflows/ubuntu-2204.yml/badge.svg)](https://github.com/gisogrimm/ov-client/actions/workflows/ubuntu-2204.yml)
[![Ubuntu 24.04 make](https://github.com/gisogrimm/ov-client/actions/workflows/ubuntu-2404.yml/badge.svg)](https://github.com/gisogrimm/ov-client/actions/workflows/ubuntu-2404.yml)
[![MacOS make 12](https://github.com/gisogrimm/ov-client/actions/workflows/macos-12.yml/badge.svg)](https://github.com/gisogrimm/ov-client/actions/workflows/macos-12.yml)
[![MacOS make 13](https://github.com/gisogrimm/ov-client/actions/workflows/macos-13.yml/badge.svg)](https://github.com/gisogrimm/ov-client/actions/workflows/macos-13.yml)
[![MacOS make 14](https://github.com/gisogrimm/ov-client/actions/workflows/macos-14.yml/badge.svg)](https://github.com/gisogrimm/ov-client/actions/workflows/macos-14.yml)

## The OVBOX system

The OVBOX system is a system for low-delay network audio communication [[1](#ref1)]. It features individual 3D virtual acoustics based on TASCAR [[2](#ref2)], integration of head tracking for dynamic binaural rendering as well as rendering of distributed moving sound sources. The system can be used for remote rehearsals as well as networked music performances. It has been used by the ORLANDOviols ensemble for a series of streaming concerts [[3](#ref3)], but also for academic research [[4](#ref4),[5](#ref5),[6](#ref6),[7](#ref7),[8](#ref8)].

The system consists of several components: the *client device*, which is this software (optionally running on a dedicated computer, see installation instructions), a *configuration server* with a web interface for the users and a REST API, and a list of *room servers*, which handle the session.

The source code of the configuration server can be found [here](https://github.com/gisogrimm/ov-webfrontend). The source code of the room server can be found [here](https://github.com/gisogrimm/ov-server).

## Installation instructions and user manual

Installation instructions can be found on the [wiki pages](https://github.com/gisogrimm/ovbox/wiki/Installation). A user manual for the ORLANDOviols configuration server can also be found on the [wiki](https://github.com/gisogrimm/ovbox/wiki).

The client software can run on Raspberry Pi (models 4B and 3B+), Ubuntu Linux and on MacOS. See installation instructions for details. A desktop client for Windows is currently under development.

## References

<a name="ref1">[1]</a> Grimm, G. (2024). Interactive low delay music and speech communication via network connections (OVBOX). Acta Acoustica, 8, 1–7. [doi:10.1051/aacus/2024011](https://doi.org/10.1051/aacus/2024011)

<a name="ref2">[2]</a> Grimm, G., Luberadzka, J. & Hohmann, V. (2019). A toolbox for rendering virtual acoustic environments in the context of audiology. Acta Acustica United with Acustica, 105(3), 566–578. [doi:10.3813/AAA.919337](https://doi.org/10.3813/AAA.919337)

<a name="ref3">[3]</a> ORLANDOviols ovbox concers 2020-2022. [youtube playlist](https://www.youtube.com/playlist?list=PLrdgCEhKvxnX0Kio9mZPLEnyKz0ldaCa1)

<a name="ref4">[4]</a> Hartwig, M., Hohmann, V. & Grimm, G. (2021). Speaking with avatars - influence of social interaction on movement behavior in interactive hearing experiments. IEEE VR 2021 Workshop: Sonic Interactions in Virtual Environments (SIVE), 94–98. [doi:10.1109/VRW52623.2021.00025](https://doi.org/10.1109/VRW52623.2021.00025)

<a name="ref5">[5]</a> Grimm, G., Kayser, H., Kothe, A. & Hohmann, V. (2023, January). Evaluation of behavior-controlled hearing devices in the lab using interactive turn-taking conversations. Proceedings of the 10th Convention of the European Acoustics Association, Forum Acusticum 2023. [doi:10.61782/fa.2023.0127](https://doi.org/10.61782/fa.2023.0127)

<a name="ref6">[6]</a> Grimm, G., Kothe, A. & Hohmann, V. (2023, January). Effect of head motion animation on immersion and conversational benefit in turn-taking conversations via telepresence in audiovisual virtual environments. Proceedings of the 10th Convention of the European Acoustics Association Forum Acusticum 2023. [doi:10.61782/fa.2023.0126](https://doi.org/10.61782/fa.2023.0126)

<a name="ref7">[7]</a> Grimm, G., Daeglau, M., Hohmann, V. & Debener, S. (2024). EEG Hyperscanning in the Internet of Sounds:Low-Delay Real-time multi-modal Transmissionusing the OVBOX. 2024 IEEE 5th International Symposium on the Internet of Sounds (IS2), 1–8. [doi:10.1109/IS262782.2024.10704205](https://doi.org/10.1109/IS262782.2024.10704205)

<a name="ref8">[8]</a> Müller, P., Haefeli, R., Schütt, J. & Ziegler, M. (2024). Telemersive Audio Systems for Online Jamming. 2024 IEEE 5th International Symposium on the Internet of Sounds (IS2), 1, 1–15. [doi:10.1109/is262782.2024.10704096](https://doi.org/10.1109/is262782.2024.10704096)