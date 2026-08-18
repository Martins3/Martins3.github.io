## 为什么 docker 中需要 GPU 的插件支持?
```txt
🧀  sudo docker info
Client:
 Version:    29.4.2
 Context:    default
 Debug Mode: false
 Plugins:
  buildx: Docker Buildx (Docker Inc.)
    Version:  0.33.0
    Path:     /usr/libexec/docker/cli-plugins/docker-buildx
  compose: Docker Compose (Docker Inc.)
    Version:  5.1.2
    Path:     /usr/libexec/docker/cli-plugins/docker-compose

Server:
 Containers: 57
  Running: 2
  Paused: 0
  Stopped: 55
 Images: 58
 Server Version: 29.4.2
 Storage Driver: overlay2
  Backing Filesystem: xfs
  Supports d_type: true
  Using metacopy: false
  Native Overlay Diff: true
  userxattr: false
 Logging Driver: json-file
 Cgroup Driver: systemd
 Cgroup Version: 2
 Plugins:
  Volume: local
  Network: bridge host ipvlan macvlan null overlay
  Log: awslogs fluentd gcplogs gelf journald json-file local splunk syslog
 CDI spec directories:
  /etc/cdi
  /run/cdi
 Discovered Devices:
  cdi: nvidia.com/gpu=0
  cdi: nvidia.com/gpu=GPU-672c39b8-f391-2e57-ad8e-33037a7e8aa5
  cdi: nvidia.com/gpu=all
 Swarm: inactive
 Runtimes: io.containerd.runc.v2 nvidia runc
 Default Runtime: runc
 Init Binary: /usr/bin/tini-static
 containerd version: 1.fc44
 runc version:
 init version:
 Security Options:
  seccomp
   Profile: builtin
  cgroupns
 Kernel Version: 7.0.8-200.fc44.x86_64
 Operating System: Fedora Linux 44 (Server Edition)
 OSType: linux
 Architecture: x86_64
 CPUs: 32
 Total Memory: 125GiB
 Name: localhost.localdomain
 ID: 51ff7388-cc45-4fdc-8e25-697558b46e07
 Docker Root Dir: /var/lib/docker
 Debug Mode: false
 HTTP Proxy: http://10.0.0.2:7890/
 HTTPS Proxy: http://10.0.0.2:7890/
 Experimental: false
 Insecure Registries:
  ::1/128
  127.0.0.0/8
 Live Restore Enabled: false
 Firewall Backend: iptables
```
