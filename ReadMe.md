# NyaTrace

一个~~聊胜于无的~~可视化路由追踪工具（ Windows 平台）

## 免责须知

我们预先构建的可执行程序并不能保证在您的设备上也能功能完整地正常执行，如果您有任何问题，您可以考虑尝试自行构建，或是提出一个 issue 。

程序使用的开发语言为 C++ ，开发环境为 Qt 5.12.12 ，构建工具支持 MinGW 7.3.0 64bit 与 MSVC 2017 64bit （之后可能会有修改）。

## 使用

1. 从 Release 页面下载预先构建的可执行文件压缩包。
2. 从 MaxMind 下载 GeoIP2-City.mmdb 和 GeoIP2-ISP.mmdb 文件，放置在可执行文件目录的 mmdb 目录下。
3. 运行程序，输入需要路由追踪的地址，开始追踪。

## 提示

1. 如果您发现除了最后一包之外其余的全都超时，您可能需要设置以下的防火墙规则，以放行 `ICMP Time-to-live Exceeded` 包和 `ICMP Destination (Port) Unreachable` 包。请以管理员权限执行以下代码：

    ```
    netsh advfirewall firewall add rule name="All ICMP v4" dir=in action=allow protocol=icmpv4:any,any
    netsh advfirewall firewall add rule name="All ICMP v6" dir=in action=allow protocol=icmpv6:any,any
    ```

2. 如果您没有购买 GeoIP2 数据库，您也可以使用 GeoLite2 （即免费版的）数据库作为替代（ GeoIP2-ISP 请用 GeoLite2-ASN 代替），将其重命名为上文指定的格式即可，需要注意相关的结果和数据可能并不如完整版丰富准确。

## 后续的开发方向

1. 集成一个地图服务
2. 跨平台兼容
