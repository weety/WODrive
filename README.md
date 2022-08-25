### 项目说明
-----------
本工程基于ODrive 0.5.1版本固件移植，支持V3.5硬件，灯哥双路驱动板，操作系统采用了RT-Thread，应用层及相关协议兼容原版。本工程为试验性质，未经过充分测试，使用时需要小心。

### 编译指导
-----------
工程编译目录：firmware/board/stm32/stm32f405

编译命令scons

scons推荐4.x.x版本

python版本推荐3.8.x，不支持python2.x版本，编译会报错

工具链推荐 gcc-arm-none-eabi-10-2020-q4-major

RT-Thread推荐用ENV工具，安装参考：[魔改版ENV](https://club.rt-thread.org/ask/article/fee92880319caa76.html)

### Repository Structure
-------------
 * **firmare**: WODrive firmware & project
 * **tools**: Python library & tools
 * **RT-Thread**: RT-Thread kernel

### Other Resources
-------------
 * [Main Website](https://www.odriverobotics.com/)
 * [User Guide](https://docs.odriverobotics.com/)
 * [Forum](https://discourse.odriverobotics.com/)
 * [Chat](https://discourse.odriverobotics.com/t/come-chat-with-us/281)
