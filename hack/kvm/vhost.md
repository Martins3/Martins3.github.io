# vhost
打开 vhost 的方法, 依次打开:
```
Symbol: VSOCKETS [=y]                                                                                                                                                                                                                                                                 
Type  : tristate                                                                                                                                                                                                                                                                      
Defined at net/vmw_vsock/Kconfig:6                                                                                                                                                                                                                                                    
  Prompt: Virtual Socket protocol                                                                                                                                                                                                                                                     
  Depends on: NET [=y]                                                                                                                                                                                                                                                                
  Location:                                                                                                                                                                                                                                                                           
    -> Networking support (NET [=y])                                                                                                                                                                                                                                                  
(5)   -> Networking options  

Symbol: VHOST_NET [=n]                                                                                                                                                                                                                                                                
Type  : tristate                                                                                                                                                                                                                                                                      
Defined at drivers/vhost/Kconfig:29                                                                                                                                                                                                                                                   
  Prompt: Host kernel accelerator for virtio net                                                                                                                                                                                                                                      
  Depends on: VHOST_MENU [=y] && NET [=y] && EVENTFD [=y] && (TUN [=n] || !TUN [=n]) && (TAP [=n] || !TAP [=n])                                                                                                                                                                       
  Location:                                                                                                                                                                                                                                                                           
    -> Device Drivers                                                                                                                                                                                                                                                                 
(3)   -> VHOST drivers (VHOST_MENU [=y])                                                                                                                                                                                                                                              
Selects: VHOST [=y]    
```

## 大致浏览代码
- https://www.redhat.com/en/blog/deep-dive-virtio-networking-and-vhost-net
- https://www.redhat.com/en/blog/introduction-virtio-networking-and-vhost-net

#### net.c
两个根本核心函数:
- handle_rx / handle_rx_net
- handle_tx / handle_tx_net

- vhost_net_open : 注册各种 handler
  vhost_virtqueue
