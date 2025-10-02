pipeline {
  agent any

  environment {
    IMAGE_NAME = "ros2_ci:humble"
    WORKDIR    = "ros2_ws/src/ros2_ci"   // path relative to repo root in Jenkins workspace
  }

  // Poll SCM each minute so a PR merge (push) triggers quickly
  triggers { pollSCM('* * * * *') }

  stages {
    stage('Checkout') {
      steps {
        checkout scm
        dir("${WORKDIR}") {
          sh 'echo "Repo at: $(pwd)"; ls -la'
        }
      }
    }

    stage('Build Docker Image') {
      steps {
        dir("${WORKDIR}") {
          sh '''
            docker build -t ${IMAGE_NAME} .
            docker image ls ${IMAGE_NAME}
          '''
        }
      }
    }

    stage('Run ROS 2 Waypoints Tests') {
      steps {
        dir("${WORKDIR}") {
          // No "-it" (non-interactive in Jenkins). Use host networking for DDS discovery.
          sh '''
            set -e
            rm -rf test_artifacts && mkdir -p test_artifacts

            docker run --rm --network=host ${IMAGE_NAME} bash -lc "/test_waypoints.sh" || TEST_FAILED=1

            # Copy ament/colcon JUnit results out of the image
            CID=$(docker create ${IMAGE_NAME} true)
            docker cp "$CID":/root/ros2_ws/build/fastbot_waypoints/test_results ./test_artifacts/ || true
            docker rm "$CID" >/dev/null

            if [ "${TEST_FAILED:-0}" = "1" ]; then exit 1; fi
          '''
        }
      }
      post {
        always {
          dir("${WORKDIR}") {
            junit allowEmptyResults: true, testResults: 'test_artifacts/test_results/**/*.xml'
            archiveArtifacts artifacts: 'test_artifacts/**', onlyIfSuccessful: false
          }
        }
      }
    }
  }

  post {
    success { echo '✅ ROS 2 CI passed; Gazebo shut down cleanly.' }
    failure { echo '❌ ROS 2 CI failed. Check Console Output + JUnit.' }
  }
}
