# @todo 将这个整理为脚本
# wget http://localhost:8080/jnlpJars/jenkins-cli.jar -o /tmp/jenkins-cli.jar

java -jar /tmp/jenkins-cli.jar -s http://localhost:8080/ -webSocket -auth martins3:jenkins list-jobs
