# CSC4303 Assignment-2: Socket Programming

TCP 套接字编程作业：实现**回显服务器/客户端**与**文件传输**两套程序，全部用 C 语言编写，基于 `poll()` 实现 I/O 多路复用，支持 30 个并发客户端与 50MB+ 大文件传输。

## 目录结构

| 文件 | 说明 |
|---|---|
| `socket_server.c` | 基础部分：TCP 回显服务器（poll 多路复用，backlog=3，最多 30 个并发客户端） |
| `socket_client.c` | 基础部分：TCP 客户端（连接 → 发送消息 → 接收回显 → 退出） |
| `file_server.c` | 文件传输：接收文件名与文件内容，落盘保存到当前工作目录 |
| `file_client.c` | 文件传输：发送文件名 + 文件内容，支持任意大小文件 |
| `CMakeLists.txt` | CMake 构建配置（生成 4 个可执行文件） |
| `con.sh` | 并发测试脚本：模拟 30 个客户端同时连接 socket_server |
| `con_file.sh` | 并发文件传输测试脚本（5×10MB + 2×50MB） |
| `generator.sh` | 生成测试文件（`dd` 生成随机文件） |
| `作业要求总结.md` | 作业要求中文总结（评分构成、规范、测试流程） |
| `项目代码总结.md` | 代码结构分析 |
| `完成作业步骤指南.md` | 分步完成指南 |
| `核心知识补充.md` | 套接字编程核心知识点 |
| `验证与排错手册.md` | 构建/运行/排错手册 |
| `总计划.md` / `Assignment-2_学习笔记与实证记录.md` | 学习与计划笔记 |

## 环境要求

- Unix-like 操作系统（Ubuntu 22.04 / WSL2）
- `gcc >= 9`，`cmake >= 3.10`

## 构建

```bash
mkdir build && cd build
cmake ..
make
```

## 运行与测试

### Part 1: 基础 Socket 编程（回显）

```bash
# 终端 A：启动服务器
./socket_server

# 终端 B：启动客户端（连 127.0.0.1:8080）
./socket_client

# 并发测试：30 个客户端
chmod +x con.sh && ./con.sh
```

### Part 2: 简单文件传输

```bash
# 1. 生成测试文件（在仓库根目录）
chmod +x generator.sh && ./generator.sh

# 2. 终端 A：启动文件服务器
./file_server

# 3. 终端 B：传输单个文件
./file_client ../file1.zip

# 4. 校验完整性
md5sum ../file1.zip ./file1.zip

# 5. 并发文件传输
chmod +x con_file.sh && ./con_file.sh
```

### Part 3: 大文件（>32MB）

```bash
./file_client ../bigfile1.zip
md5sum ../bigfile1.zip ./bigfile1.zip
```

## 文件传输协议

客户端 → 服务器，每个消息单元定长 `BUFFER_SIZE`（1024 字节）：

```
[ 8 字节文件大小（大端 uint64） | 文件名（填充至 1024） ]
[ 文件内容分块（64KB chunk）... ]
[ 服务器回发 "OK" 确认 ]
```

- 服务器按头部声明的大小接收**精确字节数**，不依赖对端关闭连接判断结束，因此 50MB+ 大文件也能正确传输；
- 服务器仅取文件名基名（剥离路径），防止路径穿越。

## 关键实现点

- **poll() 多路复用**：`poll_fds[0]` 固定为监听 socket，`poll_fds[i+1]` 对应 `client_socket[i]`；fd 为 0 表示空槽，poll 自动忽略；
- **错误处理约定**：与客户端连接相关的错误（accept/read 等）处理后服务器不退出；其他致命错误（socket/bind/listen/poll）才退出；`EINTR` 一律重试；
- **常量使用**：统一使用 `PORT`(8080)、`BUFFER_SIZE`(1024)、`MAX_CLIENTS`(30)、`BACKLOG`(3)，不硬编码。

## 评分构成（参考作业要求）

| 部分 | 分值 |
|---|---|
| 基础 Socket 编程（单客户端 30% + 并发 30%） | 60% |
| 简单文件传输 | 30% |
| 大文件（>32MB）传输 | 10% |

## 参考资料

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/)
- 系统调用手册：`man socket` / `man poll` / `man accept`
