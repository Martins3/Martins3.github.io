# QEMU 中的面向对象 : QOM

因为 QEMU 整个项目是 C 语言写的，但是 QEMU 处理的对象例如主板，CPU, 总线，外设实际上存在很多继承的关系。
所以，QEMU 为了方便整个系统的构建，实现了自己的一套的面向对象机制，也就是 QEMU Object Model（下面称为 QOM）。
