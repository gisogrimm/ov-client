def tascar_build_steps(stage_name) {
    // Extract components from stage_name:
    def system, arch, devenv
    (system,arch,devenv) = stage_name.split(/ *&& */) // regexp for missing/extra spaces

    // checkout ov-client from version control system, the exact same revision that
    // triggered this job on each build slave
    checkout scm

    // Avoid that artifacts from previous builds influence this build
    sh "git reset --hard && git clean -ffdx"

    // Update submodules
    sh "git submodule update --init --recursive"

    // Autodetect libs/compiler
    sh "make"

    // Package debians
    sh "make packaging"

    // Store debian packets for later retrieval by the repository manager
    stash name: (arch+"_"+system), includes: 'packaging/deb/debian/'
}

pipeline {
    agent {label "pipeline"}
    options {
        buildDiscarder(logRotator(daysToKeepStr: '7', artifactDaysToKeepStr: '7'))
    }
    stages {
        stage("build") {
            parallel {
                stage(                        "jammy && x86_64 && tascardev") {
                    agent {
                        docker {
                            image "hoertech/docker-buildenv:tascar_x86_64-linux-gcc-11"
                            label "docker_x86_64"
                            alwaysPull true
                            args "-v /home/u:/home/u --hostname docker"
                        }
                    }
                    steps {tascar_build_steps("jammy && x86_64 && tascardev")}
                }
                stage(                        "focal && x86_64 && tascardev") {
                    agent {
                        docker {
                            image "hoertech/docker-buildenv:tascar_x86_64-linux-gcc-9"
                            label "docker_x86_64"
                            alwaysPull true
                            args "-v /home/u:/home/u --hostname docker"
                        }
                    }
                    steps {tascar_build_steps("focal && x86_64 && tascardev")}
                }
                stage(                        "bionic && x86_64 && tascardev") {
                    agent {
                        docker {
                            image "hoertech/docker-buildenv:tascar_x86_64-linux-gcc-7"
                            label "docker_x86_64"
                            alwaysPull true
                            args "-v /home/u:/home/u --hostname docker"
                        }
                    }
                    steps {tascar_build_steps("bionic && x86_64 && tascardev")}
                }
                stage(                        "bionic && armv7 && tascardev") {
                    agent {
                        docker {
                            image "hoertech/docker-buildenv:tascar_armv7-linux-gcc-7"
                            label "docker_qemu"
                            alwaysPull true
                            args "-v /home/u:/home/u --hostname docker"
                        }
                    }
                    //agent {label              "bionic && armv7 && tascardev"}
                    steps {tascar_build_steps("bionic && armv7 && tascardev")}
                }
                stage(                        "bullseye && armv7 && tascardev") {
                    agent {
                        docker {
                            image "hoertech/docker-buildenv:tascar_armv7-linux-gcc-10"
                            label "docker_qemu"
                            alwaysPull true
                            args "-v /home/u:/home/u --hostname docker"
                        }
                    }
                    //agent {label              "bullseye && armv7 && tascardev"}
                    steps {tascar_build_steps("bullseye && armv7 && tascardev")}
                }
                stage(                        "bullseye && aarch64 && tascardev") {
                    agent {
                        docker {
                            image "hoertech/docker-buildenv:tascar_aarch64-linux-gcc-10"
                            label "docker_qemu"
                            alwaysPull true
                            //args "-v /home/u:/home/u --hostname docker"
                        }
                    }
                    //agent {label              "bullseye && aarch64 && tascardev"}
                    steps {tascar_build_steps("bullseye && aarch64 && tascardev")}
                }
            }
        }
        stage("artifacts") {
            agent {label "aptly"}
            // do not publish packages for any branches except these
            when { anyOf { branch 'master'; branch 'development' } }
            steps {
                // receive all deb packages from tascarpro build
                unstash "x86_64_jammy"
                unstash "x86_64_focal"
                unstash "x86_64_bionic"
                unstash "armv7_bullseye"
                unstash "aarch64_bullseye"
                unstash "armv7_bionic"

                // Copies the new debs to the stash of existing debs,
                sh "make -f htchstorage.mk storage"

                build job: "/Packaging/hoertech-aptly/$BRANCH_NAME", quietPeriod: 300, wait: false
            }
        }
    }
    post {
        failure {
            mail to: 'giso.grimm@vegri.net',
            subject: "Failed Pipeline: ${currentBuild.fullDisplayName}",
            body: "Something is wrong with ${env.BUILD_URL} ($GIT_URL)"
        }
    }
}
