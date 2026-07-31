# 连接到 pyOCD 的 gdbserver
target remote localhost:3333

# 烧录固件到开发板 (如果不需要烧录可注释掉)
# load

# 让 pyOCD 复位并暂停芯片，等待调试
monitor reset halt
