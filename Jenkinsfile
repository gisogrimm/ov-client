def tascar_build_steps(stage_name) {
    // Extract components from stage_name:
    def system, arch, devenv
    (system,arch,devenv) = stage_name.split(/ *&& */) // regexp for missing/extra spaces

    // checkout ov-client from version control system, the exact same revision that
    // triggered this job on each build slave
    checkout scm

    // install TASCAR:
    sh "DEBIAN_FRONTEND=noninteractive apt update"
    sh "DEBIAN_FRONTEND=noninteractive apt install --assume-yes libtascar-dev mhamakedeb"

    // Avoid that artifacts from previous builds influence this build
    sh "git reset --hard && git clean -ffdx"

    // Autodetect libs/compiler
    sh "make"

    // Package debians
    sh "make packaging"

    // Store debian packets for later retrieval by the repository manager
    stash name: (arch+"_"+system), includes: 'ovclient*.deb'
}

pipeline {
    agent {label "jenkinsmaster"}
    stages {
        stage("build") {
            parallel {
		//stage(                        "focal && x86_64 && tascardev") {
                //    agent {label              "focal && x86_64 && tascardev"}
                //    steps {tascar_build_steps("focal && x86_64 && tascardev")}
                //}
		stage(                        "bionic && x86_64 && tascardev") {
                    agent {label              "bionic && x86_64 && tascardev"}
                    steps {tascar_build_steps("bionic && x86_64 && tascardev")}
                }
                stage(                        "xenial && x86_64 && tascardev") {
                    agent {label              "xenial && x86_64 && tascardev"}
                    steps {tascar_build_steps("xenial && x86_64 && tascardev")}
                }
		stage(                        "bionic && armv7 && tascardev") {
                    agent {label              "bionic && armv7 && tascardev"}
                    steps {tascar_build_steps("bionic && armv7 && tascardev")}
                }
	    }
	}
	stage("artifacts") {
	    agent {label "aptly"}
	    // do not publish packages for any branches except these
	    when { anyOf { branch 'master'; branch 'development' } }
	    steps {
                // receive all deb packages from tascarpro build
                //unstash "x86_64_focal"
                unstash "x86_64_bionic"
                unstash "x86_64_xenial"
                unstash "armv7_bionic"
	
                // Copies the new debs to the stash of existing debs,
                sh "make -f htchstorage.mk storage"
	
                //build job: "/hoertech-aptly/$BRANCH_NAME", quietPeriod: 300, wait: false
	    }
	}
    }
    post {
        failure {
	    mail to: 'g.grimm@hoertech.de',
	    subject: "Failed Pipeline: ${currentBuild.fullDisplayName}",
	    body: "Something is wrong with ${env.BUILD_URL} ($GIT_URL)"
        }
    }
}
