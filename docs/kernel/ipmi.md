# 大致了解一下

- https://github.com/causten/openbmc-tutorials : 到底什么是 bmc

<p align="center">
  <img src="https://upload.wikimedia.org/wikipedia/commons/f/f2/IPMI-Block-Diagram.png" alt="drawing" align="center"/>
</p>
<p align="center">
from https://en.wikipedia.org/wiki/Intelligent_Platform_Management_Interface
</p>


<p align="center">
  <img src="./img/ipmi.png" alt="drawing" align="center"/>
</p>
<p align="center">
from https://www.supermicro.org.cn/manuals/other/IPMI_Users_Guide.pdf
</p>

## kernel doc

- https://docs.kernel.org/driver-api/ipmi.html


在 kernel 中可以找到一个这个东西:
/sys/devices/platform/ipmi_bmc.0/power
